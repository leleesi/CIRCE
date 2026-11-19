# CIRCE v.0.0.1

Medição de desempenho de jogos em C++ para Windows 11 x64, com painel Win32 e atalho **Ctrl + Home**. O instalador inclui o PresentMon 2.4.1 x64 e o gravador. Só a opção de vídeo precisa de Internet para descarregar o FFmpeg. O Windows pede elevação ao abrir o CIRCE.

## Primeiro registo

1. Fecha a versão anterior pelo X do painel. Abre `CIRCE-Setup.exe`. A primeira janela apresenta o C verde e os botões **Português** e **English**. Escolhe o idioma, e seleciona os componentes e o atalho no ambiente de trabalho. Podes voltar a executar o instalador para mudar de idioma. O painel, as legendas dos novos vídeos, os PDFs e os diagnósticos seguem a escolha. Os nomes dos perfis e as comparações guardadas funcionam nos dois idiomas.
2. Abre o jogo em **janela sem bordas** e prime **Ctrl + Home**. O modo automático seleciona a janela que estava em primeiro plano. Em **Sessão**, podes escolher outra pelo título e pelo executável. Em ecrã inteiro exclusivo, a apresentação do painel externo depende do jogo.
3. Escolhe 15, 30 ou 60 segundos e dá um nome à configuração. Em **Medição**, escolhe se queres ver FPS, CPU e RAM do sistema, ou gravar os clipes. As preferências ficam guardadas para a próxima abertura.
4. Clica em **Registar Antes**, mantém o jogo em primeiro plano e aguarda a contagem de três segundos. Altera as definições do jogo e usa **Registar Depois** para a mesma duração.
5. Em **Resultados**, consulta as duas médias, o P99 do tempo de fotograma e a variação percentual de FPS. O MP4 precisa de duas sessões com vídeo. O PDF precisa das duas medições.

Antes de gravar o primeiro clipe, escolhe uma pasta. Podes alterá-la em **Resultados → Escolher pasta…**. **Abrir destino** abre essa pasta. O CIRCE memoriza a escolha para o teu utilizador do Windows. **Exportar MP4** e **Exportar PDF** abrem **Guardar como** nessa pasta. Podes escolher outro nome ou local para cada ficheiro. Se cancelares, podes voltar à exportação quando quiseres.

Os clipes Antes e Depois e os respetivos ficheiros de tempos `.qpc` ficam juntos. Se mudares de pasta entre duas gravações, o CIRCE consegue usar os dois locais ao reabrir a comparação. Uma unidade externa tem de estar ligada para ler os clipes que lá guardaste.

A pasta de dados é `%LOCALAPPDATA%\CIRCE\Nativo`. Cada novo Antes cria uma comparação. Repetir um Depois já concluído guarda também uma nova versão do par. Podes reabrir comparações anteriores através dos ficheiros `.cad`. Mantém os ficheiros `.frames`, `.qpc` e `.mkv` ao lado da respetiva comparação quando a copiares. O histórico SQLite da edição antiga abre em modo de leitura. Para remover a aplicação, usa **Definições → Aplicações → Aplicações instaladas → CIRCE**. O desinstalador preserva as comparações e os clipes.

## Gravar e exportar

Todas as funções do CIRCE são gratuitas. Para gravar clipes, seleciona o componente de vídeo no instalador e ativa **Medição → Gravar clipes**. Podes exportar o PDF mesmo sem instalar o componente de vídeo.

Na atualização do Cadência, o instalador reaproveita a instalação anterior reconhecida. Move as comparações de `%LOCALAPPDATA%\Cadencia\Nativo` para `%LOCALAPPDATA%\CIRCE\Nativo` quando o destino está livre. Quando ambas as pastas já existem, os respetivos ficheiros ficam nos locais originais. **Abrir comparação** permite abrir os ficheiros antigos `.cad` diretamente.

## Medição e vídeo

O contador lê os eventos de apresentação do processo selecionado. Mostra a cadência dos últimos 100 ms de amostras. Identifica leituras com atraso a partir de 300 ms e indica a falta de dados recentes após dois segundos. O jogo pode renderizar a outra cadência quando o painel recebe o foco. A média de cada registo usa os intervalos completos dentro da sessão. Uma sessão precisa de cobrir pelo menos 90% do intervalo e de terminar com sucesso para ser guardada. O P99 é o percentil 99 dos intervalos entre apresentações, em milissegundos. CPU e RAM referem-se ao sistema inteiro.

A gravação usa Windows Graphics Capture na janela selecionada e Direct3D 11 para ajustar a imagem. Quando esta captura não arranca, o gravador pode usar GDI na mesma janela. Alguns jogos, conteúdos protegidos ou controladores podem impedir a captura. A codificação MPEG-4 por software pode afetar o desempenho. Escolhe 720p30, 1080p30 ou 1080p60 e usa a mesma opção nos dois registos. O MP4 final mostra os dois lados em 1280 × 360 a 30 FPS, com rótulos e valores calculados para cada instante. A exportação usa apenas o intervalo comum aos dois vídeos. O MP4 desta edição é silencioso.

**Testar no jogo** executa um ensaio temporário de três segundos. Com a gravação desligada, testa só a medição. **Relatório FPS (8 s)** compara os contadores do programa com uma segunda captura independente de PresentMon. No fim, abre `diagnostico-fps.txt` (em inglês, `fps-diagnostics.txt`) no Bloco de Notas. O relatório identifica o executável, o PID, as leituras aceites ou rejeitadas e, quando disponível, o último erro do gravador.

## Compilar e testar

Usa `native\build.cmd` para compilar no Windows com o Visual Studio 2022, as ferramentas C++ x64 e um Windows SDK recente. As [instruções de contribuição](../CONTRIBUTING.md) incluem a compilação cruzada em Linux e a bateria de testes. A página Releases disponibiliza o instalador para Windows e um ZIP separado com o projeto e a versão do PresentMon para compilar offline.

Os testes cobrem os dados de fotogramas, dois clipes gerados, exportações em PDF e vídeo nos dois idiomas, a escolha de pasta, o idioma e o instalador compilado. A recolha ETW, a captura e a sobreposição num jogo real precisam de um PC Windows com GPU.
