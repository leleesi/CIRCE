#!/bin/sh
set -eu
dir="$1/media com espaços"
mkdir -p "$dir"
ffmpeg -nostdin -v error -f lavfi -i 'color=red:s=160x90:r=2:d=1' -f lavfi -i 'color=green:s=160x90:r=2:d=1' -filter_complex_threads 1 -filter_complex '[0:v][1:v]concat=n=2:v=1:a=0' -c:v mpeg4 -threads 1 "$dir/clipe-1.mkv"
ffmpeg -nostdin -v error -f lavfi -i 'color=blue:s=160x90:r=2:d=2' -c:v mpeg4 -threads 1 "$dir/clipe-2.mkv"
"$1/session_video" "$dir"
for lang in pt en; do
    if [ "$lang" = pt ]; then
        set -- "$dir/Exportação à escolha/Vídeos finais/vídeo-pt.mp4"
        left="ANTES  100 FPS"; right="DEPOIS  50 FPS"
        title="Comparação de desempenho"; avg="FPS médios: 100,0"; after="FPS médios: 50,0"
    else
        set -- "$dir/Exportação à escolha/Vídeos finais/vídeo-en.mp4"
        left="BEFORE  100 FPS"; right="AFTER  50 FPS"
        title="Performance comparison"; avg="Average FPS: 100.0"; after="Average FPS: 50.0"
    fi
    [ "$#" = 1 ] && [ -f "$1" ] || exit 1
    file=$1
    dims=$(ffprobe -v error -select_streams v:0 -show_entries stream=width,height,r_frame_rate,nb_frames -of csv=p=0 "$file")
    [ "$dims" = "1280,360,30/1,38" ]
    pixel(){
        ffmpeg -nostdin -v error -ss "$1" -i "$file" -frames:v 1 -vf "crop=2:2:$2:180,scale=1:1" -f rawvideo -pix_fmt rgb24 -threads 1 - | od -An -tu1
    }
    set -- $(pixel 0.1 320)
    [ "$1" -gt 200 ] && [ "$2" -lt 30 ] || exit 1
    set -- $(pixel 1.1 320)
    [ "$2" -gt 90 ] && [ "$1" -lt 30 ] || exit 1
    set -- $(pixel 1.1 960)
    [ "$3" -gt 200 ] && [ "$1" -lt 30 ] || exit 1
    for at in 0.1 1.1; do
        ffmpeg -v error -ss "$at" -i "$file" -frames:v 1 -vf crop=1280:32:0:0 -f rawvideo -pix_fmt gray -threads 1 -y "$dir/frame.gray"
        "$(dirname "$dir")/video_labels" "$dir/frame.gray" "$left" "$right"
    done
    pdftotext -layout "$dir/Exportação à escolha/Relatórios/report-$lang.pdf" "$dir/pdf.txt"
    for phrase in "$title" "$avg" "$after" 'Médio' 'Ultra'; do grep -Fq "$phrase" "$dir/pdf.txt"; done
    echo "OK ($lang): MP4 1280×360, 30 FPS, 38 imagens, ambos os lados e os pixeis de [$left] e [$right]. PDF com títulos, acentos e médias."
done
