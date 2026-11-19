// A4 report, using WinAnsi characters for Portuguese names and an atomic final write.
struct PdfText {
    std::string bytes;
    void add(const char* s) { bytes += s; }
    void number(Q v) { bytes += std::to_string(v); }
    void string(const W* s) {
        char buffer[512]{};
        static const char hex[] = "0123456789ABCDEF";
        int n = WideCharToMultiByte(1252, 0, s, -1, buffer, sizeof(buffer), "?", nullptr);
        add("<");
        for (int i = 0; i < n - 1; ++i) {
            unsigned char c = (unsigned char)buffer[i];
            bytes += hex[c >> 4];
            bytes += hex[c & 15];
        }
        add(">");
    }
};
static std::string comparisonPdf(const Saved& document) {
    PdfText content, p;
    content.add("BT /F 21 Tf 48 780 Td ");
    content.string(words::pdfTitle.get());
    content.add(" Tj /F 13 Tf 0 -52 Td ");
    for (int i = 0; i < 2; ++i) {
        Text<128> title;
        title.add(i ? words::afterSide.get() : words::beforeSide.get());
        title.add(document.names[i]);
        content.string(title.buf);
        content.add(" Tj 0 -24 Td ");
        Text<192> line;
        line.add(words::averageFps.get());
        line.decimal(document.summary[i].fps);
        line.add(L"       P99: ");
        line.decimal(document.summary[i].p99);
        line.add(L" ms");
        content.string(line.buf);
        content.add(" Tj 0 -46 Td ");
    }
    Text<80> durationLine;
    durationLine.add(words::sessionDuration.get());
    durationLine.number(document.duration);
    durationLine.add(L" s");
    content.string(durationLine.buf);
    content.add(" Tj ET");
    size_t x[6]{};
    p.add("%PDF-1.4\n");
    x[1] = p.bytes.size();
    p.add("1 0 obj <</Type/Catalog/Pages 2 0 R>> endobj\n");
    x[2] = p.bytes.size();
    p.add("2 0 obj <</Type/Pages/Count 1/Kids[3 0 R]>> endobj\n");
    x[3] = p.bytes.size();
    p.add("3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 595 842]/Resources<</Font<</F 4 0 R>>>>/Contents 5 "
          "0 R>> endobj\n");
    x[4] = p.bytes.size();
    p.add("4 0 obj <</Type/Font/Subtype/Type1/BaseFont/Helvetica/Encoding/WinAnsiEncoding>> endobj\n");
    x[5] = p.bytes.size();
    p.add("5 0 obj <</Length ");
    p.number(content.bytes.size());
    p.add(">> stream\n");
    p.bytes += content.bytes;
    p.add("\nendstream endobj\n");
    auto start = p.bytes.size();
    p.add("xref\n0 6\n0000000000 65535 f \n");
    for (int i = 1; i <= 5; ++i) {
        char row[32];
        snprintf(row, sizeof(row), "%010llu 00000 n \n", (Q)x[i]);
        p.add(row);
    }
    p.add("trailer <</Size 6/Root 1 0 R>>\nstartxref\n");
    p.number(start);
    p.add("\n%%EOF\n");
    return p.bytes;
}
static bool writePdf(const Saved& comparison, const ExportTarget& target) {
    auto pdf = comparisonPdf(comparison);
    Text<2048> temp;
    H file = reserveExportTemp(target, L".part.pdf", temp);
    if (file == bad) return false;
    bool ok = writeAll(file, pdf.data(), U(pdf.size())) && FlushFileBuffers(file);
    CloseHandle(file);
    if (ok) ok = publishExport(temp, target);
    if (!ok) DeleteFileW(temp.buf);
    return ok;
}
static void exportPdf() {
    if (stage || exportThread) return;
    if (!stats[0].n || !stats[1].n) {
        say(words::pdfRequiresRuns.get());
        return;
    }
    const Saved comparison = saved;
    ExportTarget target;
    if (!selectExport(platform::OutputKind::Pdf, target)) return;
    if (writePdf(comparison, target)) reportSaved(words::pdfSaved.get(), target);
    else say(words::pdfWriteFailed.get());
}
