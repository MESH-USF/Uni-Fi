# Exporting the technical walkthrough

The authoritative text is [TECHNICAL_WALKTHROUGH.md](TECHNICAL_WALKTHROUGH.md).
Its formatted exports are [Word](Uni-Fi-Technical-Walkthrough.docx) and
[PDF](Uni-Fi-Technical-Walkthrough.pdf). Regenerate both after source changes;
editing Word alone does not update the Markdown or PDF.

The exporter preserves prose, tables, lists, directory trees and protocol code
blocks. It replaces ten Mermaid blocks with numbered rendered charts and adds
a cover, clickable reading guide, headers and page numbers. Wide charts use
landscape pages. Two overview flowcharts are reflowed top-to-bottom for print;
their nodes, connections and labels are unchanged. Diagram label line breaks
and state-transition punctuation were corrected during visual checking.

## Dependencies

Use Node.js, Python with `python-docx`, a Chromium browser controlled by
Playwright CLI, and LibreOffice for PDF conversion. The checked exports used
Mermaid 11.4.1, Marked 13.0.3, python-docx 1.2.0 and LibreOffice 26.2. Install
temporary document dependencies outside the firmware/app dependency trees:

```bash
unifi_doc_tools=$(mktemp -d /tmp/unifi-doc-tools.XXXXXX)
npm install --prefix "$unifi_doc_tools" mermaid@11.4.1 marked@13.0.3
python3 -m venv "$unifi_doc_tools/venv"
"$unifi_doc_tools/venv/bin/pip" install python-docx==1.2.0
```

## Render the charts

From the repository root, start the local-only renderer and keep it running:

```bash
node scripts/render-unifi-figures.mjs \
  --modules "$unifi_doc_tools/node_modules" \
  --ast /tmp/unifi-document-ast.json --port 8767
```

It produces a Marked lexer AST and serves a rendering page on
`http://127.0.0.1:8767`. In a separate terminal, set `unifi_doc_tools` to the
same temporary directory before the export step, and use Playwright CLI to open
that page and capture each chart. Run the following commands in the same
browser session (use the installed `playwright-cli` wrapper if applicable):

```bash
mkdir -p output/playwright/unifi-document
npx --package @playwright/cli playwright-cli open http://127.0.0.1:8767
npx --package @playwright/cli playwright-cli run-code 'async page => {
  await page.waitForFunction(() => window.renderComplete || window.renderError);
  const error = await page.evaluate(() => window.renderError);
  if (error) throw Error(error);
  await page.setViewportSize({width: 2400, height: 1700});
  for (let i = 1; i <= 10; i++) {
    const id = "figure-" + String(i).padStart(2, "0");
    await page.locator("#" + id).screenshot({
      path: "output/playwright/unifi-document/" + id + ".png", scale: "css"
    });
  }
}'
npx --package @playwright/cli playwright-cli console
npx --package @playwright/cli playwright-cli close
```

Generated screenshots are ignored by Git. Check there are ten figures, no
rendering errors, no clipped labels and no unintended state nodes. Stop the
renderer with Ctrl-C after capture.

## Export and check

```bash
"$unifi_doc_tools/venv/bin/python" scripts/export-unifi-docx.py \
  --ast /tmp/unifi-document-ast.json \
  --figures output/playwright/unifi-document \
  --output docs/Uni-Fi-Technical-Walkthrough.docx

unifi_lo_profile=$(mktemp -d /tmp/unifi-lo-profile.XXXXXX)
libreoffice "-env:UserInstallation=file://$unifi_lo_profile" \
  --headless --convert-to pdf --outdir docs \
  docs/Uni-Fi-Technical-Walkthrough.docx
```

Review representative PDF pages, especially trees, packet-layout tables and
wide charts. Confirm all ten figure captions and all sections remain present.
Relative source-file links are meaningful in this repository; use the
Markdown version for navigating code online. Charts embedded in Word are
images; modify their Mermaid source and regenerate to change their contents.

The exports describe the existing GPS-free prototype and its outstanding
hardware/mobile validation. Creating a document does not establish additional
firmware functionality, RF reliability or emergency-service certification.

## Quick guide export

The shorter high-level guide can be exported without Mermaid rendering:

```bash
PYTHONPATH=/tmp/unifi-doc-python python3 scripts/export-unifi-quick-guide.py \
  docs/UNIFI_QUICK_GUIDE.md docs/Uni-Fi-Quick-Guide.docx
libreoffice --headless --convert-to pdf --outdir docs docs/Uni-Fi-Quick-Guide.docx
```
