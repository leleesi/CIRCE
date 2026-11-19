<p align="center"><img src="assets/CIRCE.png" width="108" height="108" alt="CIRCE"></p>

# CIRCE

O que muda nos FPS quando baixas as sombras ou sobes a resolução? Joga a mesma cena duas vezes e compara os resultados.

O CIRCE é uma ferramenta de benchmark em C++ para **Windows 11 x64**. Carrega em **Ctrl + Home** dentro do jogo para escolher a janela, registar os dois testes e comparar o desempenho. O instalador, o painel, as legendas e o relatório PDF estão disponíveis em português e inglês.

[Descarregar para Windows](https://github.com/leleesi/CIRCE/releases/latest/download/CIRCE-Setup.exe) · [Releases](https://github.com/leleesi/CIRCE/releases) · [English](README.md) · [Guia de utilização](docs/guia-pt.md) · [Notas da versão](docs/releases/v0.0.1.md) · [Reportar erro](https://github.com/leleesi/CIRCE/issues/new/choose) · [Como contribuir](CONTRIBUTING.md)

Criado e mantido por [@leleesi](https://github.com/leleesi). As versões oficiais estão nas [Releases de leleesi/CIRCE](https://github.com/leleesi/CIRCE/releases). Se distribuíres uma versão modificada, identifica o teu fork e quem o mantém.

## Instalar e fazer a primeira comparação

1. Abre **Releases** no repositório do GitHub e descarrega **CIRCE-Setup.exe** da versão **v0.0.1**. Escolhe o idioma. Podes selecionar o componente de vídeo e o atalho no ambiente de trabalho.
2. Abre o CIRCE, aceita o pedido de administrador do Windows e inicia o jogo em **janela sem margens**. Carrega em **Ctrl + Home** e confirma o jogo em **Sessão**.
3. Escolhe 15, 30 ou 60 segundos. Usa **Registar Antes**, joga a cena, altera uma definição e repete a cena com **Registar Depois**.
4. Em **Resultados**, compara as medições e escolhe onde guardar o PDF ou o MP4 lado a lado. Todas as funções são gratuitas, incluindo as exportações de vídeo e PDF.

## O que podes medir

| Durante o jogo | Depois dos dois testes |
| --- | --- |
| FPS da janela selecionada, medidos pelo PresentMon | FPS médios, P99 do tempo de fotograma e variação percentual |
| Utilização de CPU e RAM do sistema | Relatório em PDF no idioma escolhido |
| Clipes opcionais da janela do jogo | MP4 lado a lado, sem áudio, com os FPS de cada registo |

O [exemplo de PDF](docs/examples/comparacao-pt.pdf) foi exportado pelo CIRCE a partir de duas sessões com dados sintéticos.

O componente de vídeo descarrega o FFmpeg durante a instalação. A medição de FPS e a exportação de PDF funcionam offline com o PresentMon incluído. O [guia](docs/guia-pt.md) explica os perfis de captura, as pastas de destino, o diagnóstico de FPS e o modo de ecrã inteiro exclusivo.

## Código e licença

Para compilar no Windows, executa `native\build.cmd` com o Visual Studio 2022, as ferramentas C++ x64 e um Windows SDK recente. As [instruções de contribuição](CONTRIBUTING.md) incluem a compilação em Linux, os testes e a organização das pastas.

A [licença MIT](LICENSE) cobre o código, a documentação e os recursos visuais originais do CIRCE. O [PresentMon](https://github.com/GameTechDev/PresentMon/releases/tag/v2.4.1) inclui os seus [avisos de terceiros](vendor/PresentMon/THIRD_PARTY.txt). O componente de vídeo instala os avisos da [distribuição FFmpeg](https://github.com/FFmpeg-Builds/FFmpeg-Builds/releases).
