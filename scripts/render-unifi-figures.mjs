#!/usr/bin/env node
// Local-only chart rendering page and Markdown AST for the document exporter.
import fs from 'node:fs';
import path from 'node:path';
import http from 'node:http';
import { pathToFileURL } from 'node:url';

const options = Object.fromEntries(process.argv.slice(2).reduce((pairs, value, i, args) => {
  if (value.startsWith('--')) pairs.push([value.slice(2), args[i + 1]]);
  return pairs;
}, []));
if (!options.modules || !options.ast) {
  throw new Error('Usage: node scripts/render-unifi-figures.mjs --modules <node_modules> --ast <json> [--port 8767]');
}
const modules = path.resolve(options.modules);
const source = fs.readFileSync('docs/TECHNICAL_WALKTHROUGH.md', 'utf8');
const { marked } = await import(pathToFileURL(path.join(modules, 'marked/lib/marked.esm.js')));
const tokens = marked.lexer(source);
fs.writeFileSync(options.ast, JSON.stringify(tokens, null, 2));
const diagrams = tokens.filter((token) => token.type === 'code' && token.lang === 'mermaid');
const captions = [
  'System overview: independent radios and optional companions',
  'Three protocol layers: application, companion and radio',
  'Build inheritance before and after the prototype',
  'Firmware module responsibilities and dependencies',
  'Device startup and provisioning',
  'Browser connection handshake and queue draining',
  'Radio reception, state update and companion collection',
  'Standalone SOS and matching human receipt sequence',
  'Current own distress state transitions',
  'Future GPS and Flutter map data ownership',
];
const data = JSON.stringify(diagrams.map((diagram, i) => ({
  number: i + 1, caption: captions[i],
  // Print reflow changes layout only; all nodes, arrows and labels are retained.
  source: diagram.text.replace(/^flowchart LR/m, 'flowchart TB').replaceAll('\\n', '<br/>'),
}))).replaceAll('<', String.fromCharCode(92) + 'u003c');
const html = `<!doctype html><html><head><meta charset="utf-8">
<title>Uni-Fi document figures</title><link rel="icon" href="data:,">
<style>
body{font-family:Arial,sans-serif;background:#f3f6fa;color:#162b42;margin:0;padding:32px}
h1{margin:0 0 28px} section{background:white;padding:28px;margin:24px auto;border:1px solid #dbe3ed;width:max-content;max-width:2200px}
h2{font-size:20px;margin:0 0 24px}.figure{display:inline-block;background:white;padding:20px}
.figure svg{max-width:none!important;width:auto!important;height:auto!important;display:block}
.caption{margin:12px 0 0;color:#53677c}#status{font-weight:bold}
</style></head><body><h1>Uni-Fi technical walkthrough — document figures</h1>
<p id="status">Rendering diagrams…</p><main id="figures"></main>
<script type="module">
import mermaid from '/mermaid/mermaid.esm.mjs';
mermaid.initialize({startOnLoad:false,securityLevel:'strict',theme:'base',
  themeVariables:{fontFamily:'Arial, sans-serif',fontSize:'20px',primaryColor:'#e9f1fb',primaryTextColor:'#162b42',primaryBorderColor:'#6685ad',lineColor:'#53677c',secondaryColor:'#eef6f3',tertiaryColor:'#ffffff'},
  flowchart:{useMaxWidth:false,nodeSpacing:28,rankSpacing:36,htmlLabels:true},
  sequence:{useMaxWidth:false,wrap:true,actorMargin:35,messageMargin:28},
  state:{useMaxWidth:false}});
window.figureInfo=[];window.renderError=null;
try {
  for(const figure of ${data}){
    const section=document.createElement('section');
    const title=document.createElement('h2');title.textContent='Figure '+figure.number+'. '+figure.caption;
    const container=document.createElement('div');container.className='figure';container.id='figure-'+String(figure.number).padStart(2,'0');
    const rendered=await mermaid.render('chart-'+figure.number,figure.source);
    container.innerHTML=rendered.svg;
    const svg=container.querySelector('svg');const view=svg.viewBox.baseVal;
    svg.style.width=Math.ceil(view.width)+'px';svg.style.height=Math.ceil(view.height)+'px';
    svg.setAttribute('width',Math.ceil(view.width));svg.setAttribute('height',Math.ceil(view.height));
    section.append(title,container);document.querySelector('#figures').append(section);
    window.figureInfo.push({number:figure.number,caption:figure.caption,width:view.width,height:view.height});
  }
  document.querySelector('#status').textContent='All '+window.figureInfo.length+' diagrams rendered';
  window.renderComplete=true;
}catch(error){window.renderError=String(error);document.querySelector('#status').textContent=String(error);}
</script></body></html>`;

const dist = path.join(modules, 'mermaid/dist');
const server = http.createServer((request, response) => {
  const url = new URL(request.url, 'http://localhost');
  if (url.pathname === '/') {
    response.setHeader('Content-Type', 'text/html; charset=utf-8');
    response.end(html);
    return;
  }
  if (url.pathname.startsWith('/mermaid/')) {
    const target = path.resolve(dist, '.' + url.pathname.slice('/mermaid'.length));
    if (!target.startsWith(dist + path.sep) || !fs.existsSync(target) || !fs.statSync(target).isFile()) {
      response.writeHead(404);response.end('Not found');return;
    }
    response.setHeader('Content-Type', target.endsWith('.json') ? 'application/json' : 'text/javascript');
    fs.createReadStream(target).pipe(response);
    return;
  }
  response.writeHead(404);response.end('Not found');
});
server.listen(Number(options.port || 8767), '127.0.0.1', () => {
  console.log(`Local figure page: http://127.0.0.1:${options.port || 8767}`);
  console.log(`AST: ${options.ast}; diagrams: ${diagrams.length}`);
});
