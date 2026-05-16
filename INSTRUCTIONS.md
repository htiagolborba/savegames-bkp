# Prompt para Antigravity — Game Save Backup Tool em C++

## Objetivo do projeto

Crie um aplicativo Windows em **C++** chamado **Game Save Backup Tool**.

O objetivo do app é localizar automaticamente savegames de jogos instalados no computador e fazer backup deles em uma pasta escolhida pelo usuário. O programa deve ter uma **interface simples**, gerar um `.exe` executável no Windows e manter um arquivo `.md` na raiz da pasta de backup com o histórico de todos os backups realizados.

Este projeto nasceu de uma necessidade real: evitar perda de savegames após formatação, troca de SSD/NVMe, reinstalação do Windows ou erro em particionamento.

---

## Requisitos principais

### 1. Plataforma

- Sistema operacional: **Windows 10/11**
- Linguagem: **C++**
- Gerar um arquivo final `.exe`
- O projeto deve ser fácil de compilar no Windows
- Preferência por uma solução simples, sem dependências pesadas

Sugestões aceitáveis de UI:
- Win32 API nativa
- Qt, se for fácil configurar
- Dear ImGui, se fizer sentido
- Outra UI simples, desde que compile para `.exe`

Prioridade: simplicidade, estabilidade e facilidade de uso.

---

## 2. Interface do usuário

Crie uma UI simples com:

### Tela principal

Elementos necessários:

1. Título:
   - `Game Save Backup Tool`

2. Botão:
   - `Choose Backup Folder`

3. Campo mostrando a pasta escolhida pelo usuário.

4. Botão:
   - `Scan for Savegames`

5. Lista dos jogos/save folders encontrados.
   - Mostrar:
     - Nome do jogo
     - Caminho original do save
     - Status: Found / Not found / Backed up / Error

6. Botão:
   - `Backup Now`

7. Área de log/status:
   - Mostrar mensagens como:
     - `Scanning save locations...`
     - `Found Resident Evil 4 Remake save folder`
     - `Backup completed successfully`
     - `Backup failed for X: reason`

8. Botão opcional:
   - `Open Backup Folder`

---

## 3. Fluxo esperado do app

O funcionamento deve ser:

1. O usuário abre o `.exe`.
2. O app pergunta/permite escolher onde salvar os backups.
3. O usuário escolhe uma pasta base, por exemplo:

```text
D:\GameSaveBackups
```

4. O app escaneia locais comuns de savegames no Windows.
5. O app lista todos os jogos/save folders encontrados.
6. Quando o usuário clicar em `Backup Now`, o app deve criar automaticamente uma estrutura de pastas assim:

```text
D:\GameSaveBackups\
  Resident Evil 4 Remake\
    savegames\
      2026-05-15_22-40-33\
        [arquivos e pastas originais do save]

  Cyberpunk 2077\
    savegames\
      2026-05-15_22-40-33\
        [arquivos e pastas originais do save]
```

Ou seja:

```text
[pasta escolhida pelo usuário]\[nome do jogo]\savegames\[data e hora da execução]\
```

Cada vez que o app for executado e fizer backup, ele deve criar uma nova subpasta dentro de `savegames` com data e hora atual.

Formato sugerido para data/hora:

```text
YYYY-MM-DD_HH-mm-ss
```

Exemplo:

```text
2026-05-15_22-40-33
```

---

## 4. Arquivo de histórico em Markdown

O app deve criar e atualizar automaticamente um arquivo `.md` na raiz da pasta de backup escolhida pelo usuário.

Nome do arquivo:

```text
backup-history.md
```

Exemplo de localização:

```text
D:\GameSaveBackups\backup-history.md
```

Esse arquivo deve ser atualizado toda vez que o usuário executar um backup.

### Conteúdo esperado do `backup-history.md`

Estrutura sugerida:

```md
# Game Save Backup History

## 2026-05-15 22:40:33

Backup destination:

D:\GameSaveBackups

### Games backed up

| Game | Source Path | Backup Path | Status |
|---|---|---|---|
| Resident Evil 4 Remake | C:\Program Files (x86)\Steam\userdata\123456\2050650 | D:\GameSaveBackups\Resident Evil 4 Remake\savegames\2026-05-15_22-40-33 | Success |
| Cyberpunk 2077 | C:\Users\Tiago\Saved Games\CD Projekt Red\Cyberpunk 2077 | D:\GameSaveBackups\Cyberpunk 2077\savegames\2026-05-15_22-40-33 | Success |

### Errors

None.
```

Se houver erro, registrar:

```md
### Errors

- Failed to backup Game X
  - Source: C:\...
  - Reason: Access denied
```

O app não deve apagar o histórico antigo. Ele deve apenas adicionar uma nova seção no topo ou no final do arquivo.

Preferência: adicionar as entradas novas no topo, logo abaixo do título, para o histórico mais recente aparecer primeiro.

---

## 5. Locais que o app deve escanear

O app deve procurar saves em locais comuns do Windows.

Use variáveis de ambiente em vez de caminhos fixos quando possível.

Locais importantes:

```text
%USERPROFILE%\Documents
%USERPROFILE%\Saved Games
%APPDATA%
%LOCALAPPDATA%
%LOCALAPPDATA%\..\LocalLow
%PROGRAMFILES(X86)%\Steam\userdata
%PROGRAMFILES%\Steam\userdata
%PROGRAMFILES(X86)%\GOG Galaxy\Games
%PROGRAMFILES(X86)%\Ubisoft\Ubisoft Game Launcher\savegames
%PROGRAMFILES%\EA Games
```

Também verificar se existem essas pastas:

```text
C:\Users\[current-user]\Documents
C:\Users\[current-user]\Saved Games
C:\Users\[current-user]\AppData\Roaming
C:\Users\[current-user]\AppData\Local
C:\Users\[current-user]\AppData\LocalLow
```

---

## 6. Detecção de jogos

Implemente um sistema híbrido:

### A. Base de dados interna de jogos conhecidos

Crie um arquivo ou estrutura interna contendo jogos conhecidos e seus possíveis caminhos de save.

Pode ser JSON, TOML, INI ou uma estrutura C++ simples.

Preferência: usar um arquivo editável chamado:

```text
known_games.json
```

Esse arquivo deve ficar junto do `.exe`, para que eu possa adicionar novos jogos depois.

Exemplo:

```json
[
  {
    "name": "Resident Evil 4 Remake",
    "paths": [
      "%PROGRAMFILES(X86)%\\Steam\\userdata\\*\\2050650",
      "%USERPROFILE%\\Documents\\My Games\\Capcom\\RE4"
    ]
  },
  {
    "name": "Resident Evil Village",
    "paths": [
      "%PROGRAMFILES(X86)%\\Steam\\userdata\\*\\1196590"
    ]
  },
  {
    "name": "Cyberpunk 2077",
    "paths": [
      "%USERPROFILE%\\Saved Games\\CD Projekt Red\\Cyberpunk 2077"
    ]
  },
  {
    "name": "The Witcher 3",
    "paths": [
      "%USERPROFILE%\\Documents\\The Witcher 3"
    ]
  }
]
```

O app deve expandir:
- variáveis como `%USERPROFILE%`
- wildcards como `*`
- caminhos relativos quando necessário

### B. Escaneamento genérico

Além dos jogos conhecidos, o app deve procurar por pastas com nomes comuns relacionados a saves:

Palavras-chave úteis:

```text
save
saves
savedata
savegame
savegames
profile
profiles
user
userdata
slot
checkpoint
autosave
```

Mas cuidado: não copiar o disco inteiro. O app deve limitar o scan para evitar backups gigantes.

Sugestão:
- Escanear somente até uma profundidade razoável, por exemplo 4 ou 5 níveis.
- Ignorar pastas muito grandes, por exemplo acima de 2 GB, a menos que estejam na base de jogos conhecidos.
- Ignorar caches, logs, shader cache e pastas temporárias.

Pastas a ignorar:

```text
cache
shadercache
logs
log
temp
tmp
crash
crashes
screenshots
videos
movies
binaries
bin
engine
node_modules
```

---

## 7. Evitar duplicatas

O app deve evitar copiar o mesmo save duas vezes.

Se dois caminhos apontarem para a mesma pasta ou se uma pasta estiver contida dentro de outra já detectada, manter apenas a melhor opção.

Exemplo:

```text
C:\Users\Tiago\Saved Games\Game X
C:\Users\Tiago\Saved Games\Game X\Saves
```

Nesse caso, escolher a pasta mais apropriada, provavelmente a pasta raiz do jogo, desde que não seja grande demais.

---

## 8. Segurança

O programa não deve deletar, mover ou modificar os saves originais.

Ele deve apenas copiar.

Regras importantes:

- Nunca apagar arquivos originais.
- Nunca sobrescrever backups antigos.
- Sempre criar uma nova pasta com timestamp.
- Se a pasta de destino já existir, adicionar um sufixo:
  - `_1`
  - `_2`
  - `_3`

Exemplo:

```text
2026-05-15_22-40-33_1
```

---

## 9. Cópia dos arquivos

Ao fazer backup:

- Copiar recursivamente todos os arquivos e subpastas do save original.
- Preservar a estrutura interna.
- Preservar nomes de arquivos.
- Se possível, preservar timestamps dos arquivos.
- Se algum arquivo estiver bloqueado, registrar erro e continuar com os demais.
- Não interromper o backup inteiro por causa de um erro em um jogo.

---

## 10. Configuração persistente

O app deve lembrar a última pasta de backup escolhida.

Criar um arquivo de configuração simples junto do `.exe`:

```text
config.json
```

Exemplo:

```json
{
  "lastBackupFolder": "D:\\GameSaveBackups"
}
```

Quando o app abrir novamente, carregar essa pasta automaticamente.

---

## 11. Log da execução

Além do `backup-history.md`, criar opcionalmente um log técnico:

```text
backup-log.txt
```

Pode ficar na raiz da pasta de backup.

Esse log deve conter mensagens detalhadas para debug.

Exemplo:

```text
[2026-05-15 22:40:33] App started
[2026-05-15 22:40:36] Scanning known game paths
[2026-05-15 22:40:37] Found Resident Evil 4 Remake at C:\...
[2026-05-15 22:40:45] Backup completed
```

---

## 12. Estrutura sugerida do projeto

Crie o projeto com uma organização parecida com esta:

```text
GameSaveBackupTool/
  README.md
  CMakeLists.txt
  known_games.json
  config.json
  src/
    main.cpp
    App.cpp
    App.h
    BackupManager.cpp
    BackupManager.h
    GameScanner.cpp
    GameScanner.h
    ConfigManager.cpp
    ConfigManager.h
    MarkdownHistory.cpp
    MarkdownHistory.h
    Utils.cpp
    Utils.h
  build/
  dist/
    GameSaveBackupTool.exe
```

Se usar outra estrutura por necessidade da UI escolhida, tudo bem, mas manter o projeto limpo e documentado.

---

## 13. README.md

Crie um `README.md` explicando:

- O que o app faz
- Como compilar
- Como executar
- Como editar `known_games.json`
- Como funciona a estrutura dos backups
- Onde fica o `backup-history.md`
- Limitações conhecidas

---

## 14. Build e compilação

Configurar o projeto para compilar em Windows.

Preferência:

```text
CMake + MSVC
```

O projeto deve ter um `CMakeLists.txt`.

Incluir instruções no README:

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

O `.exe` final deve ficar em uma pasta fácil de encontrar, preferencialmente:

```text
dist\GameSaveBackupTool.exe
```

---

## 15. Critérios de aceite

O projeto estará pronto quando:

- O app abrir com UI simples.
- O usuário conseguir escolher a pasta de backup.
- O app conseguir escanear savegames conhecidos.
- O app conseguir detectar pelo menos alguns saves comuns.
- O app conseguir fazer backup sem sobrescrever backups antigos.
- A estrutura de pasta for criada corretamente:

```text
[pasta escolhida]\[nome do jogo]\savegames\[timestamp]\
```

- O arquivo `backup-history.md` for criado e atualizado a cada execução.
- O app lembrar a última pasta usada.
- O projeto compilar para `.exe`.
- O README explicar como compilar e usar.

---

## 16. Primeira versão esperada

Para a primeira versão, priorize:

1. UI simples funcional
2. Escolha de pasta de destino
3. `known_games.json`
4. Scan de jogos conhecidos
5. Backup com timestamp
6. `backup-history.md`
7. Geração de `.exe`

Depois, em versões futuras, podemos melhorar o scan automático genérico.

---

## 17. Observações importantes

- Não tentar fazer um software perfeito logo na primeira versão.
- Priorizar segurança dos dados.
- Nunca deletar arquivos.
- Nunca modificar save original.
- Sempre registrar o que foi feito.
- O app precisa ser simples o suficiente para eu abrir, clicar em backup e pronto.
- O foco é proteger saves antes de formatar, mexer em partições, trocar SSD ou reinstalar Windows.

---

## 18. Resultado final esperado

Entregar um projeto C++ completo que gere um executável Windows:

```text
GameSaveBackupTool.exe
```

Com arquivos auxiliares:

```text
known_games.json
config.json
README.md
backup-history.md
backup-log.txt
```

O app deve permitir que eu rode manualmente sempre que quiser e faça backup seguro dos meus savegames.
