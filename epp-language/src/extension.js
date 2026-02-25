const vscode = require("vscode");
const fs = require("fs");
const path = require("path");

const KEYWORDS = [
  "var",
  "func",
  "class",
  "if",
  "else",
  "while",
  "return",
  "import",
  "from",
  "this",
  "true",
  "false",
  "null",
  "and",
  "or",
  "not",
];

const BUILTIN_SIGNATURES = {
  print: { sig: "print(...)", doc: "Imprime valores en consola." },
  input: { sig: "input(mensaje)", doc: "Lee una linea de texto." },
  int: { sig: "int(valor)", doc: "Convierte a entero." },
  loadlib: { sig: "loadlib(ruta_dll)", doc: "Carga libreria nativa C/C++." },
};

const STDLIB_SIGNATURES = {
  HTTPServer: { sig: "HTTPServer(port)", doc: "Servidor HTTP programable." },
  HTTPRequest: { sig: "HTTPRequest(client_id)", doc: "Request parseada de un cliente HTTP." },
  HTTPResponse: { sig: "HTTPResponse(client_id)", doc: "Respuesta HTTP para un cliente activo." },
  server: { sig: "server(port)", doc: "Crea servidor HTTP (wrapper)." },
  create_server: { sig: "create_server(port)", doc: "Alias de server(port)." },
  request: { sig: "request(client_id)", doc: "Crea objeto request (wrapper)." },
  response: { sig: "response(client_id)", doc: "Crea objeto response (wrapper)." },
  serve_once: { sig: "serve_once(port, body)", doc: "Compat: atiende 1 request y termina." },
  serve_once_status: { sig: "serve_once_status(port, body, status)", doc: "Compat con status custom." },
  serve: { sig: "serve(port, body, max_requests)", doc: "Compat: atiende N requests." },
  serve_status: { sig: "serve_status(port, body, max_requests, status)", doc: "Compat con status custom." },
  serve_forever: { sig: "serve_forever(port, body)", doc: "Servidor continuo." },
  serve_forever_status: { sig: "serve_forever_status(port, body, status)", doc: "Servidor continuo con status." },
};

const SYMBOL_RE = /[A-Za-z_]\w*/g;
const parsedCache = new Map();

function makeRange(line, start, end) {
  return new vscode.Range(new vscode.Position(line, start), new vscode.Position(line, end));
}

function addDiagnostic(diags, severity, line, start, end, message, code) {
  const d = new vscode.Diagnostic(makeRange(line, start, end), message, severity);
  d.source = "epp";
  d.code = code;
  diags.push(d);
}

function isEscaped(text, idx) {
  let backslashes = 0;
  for (let i = idx - 1; i >= 0 && text[i] === "\\"; --i) backslashes++;
  return backslashes % 2 === 1;
}

function stripLineCommentAware(raw) {
  let inString = false;
  for (let i = 0; i < raw.length; i++) {
    const ch = raw[i];
    const next = i + 1 < raw.length ? raw[i + 1] : "";
    if (ch === '"' && !isEscaped(raw, i)) inString = !inString;
    if (!inString && ch === "/" && next === "/") {
      return raw.slice(0, i);
    }
  }
  return raw;
}

function quoteArg(value) {
  if (!value) return '""';
  return `"${String(value).replace(/"/g, '\\"')}"`;
}

function getWorkspaceFolderForUri(uri) {
  const folder = uri ? vscode.workspace.getWorkspaceFolder(uri) : null;
  if (folder) return folder.uri.fsPath;
  const first = vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders[0];
  return first ? first.uri.fsPath : process.cwd();
}

function detectCliPath(workspaceRoot) {
  const cfg = vscode.workspace.getConfiguration("eppLanguage");
  const configured = String(cfg.get("cliPath", "auto") || "auto").trim();
  if (configured && configured.toLowerCase() !== "auto") return configured;

  const localCandidates = [
    path.join(workspaceRoot, "build", "Release", "epp.exe"),
    path.join(workspaceRoot, "build", "epp"),
  ];
  for (const p of localCandidates) {
    if (fs.existsSync(p)) return p;
  }
  return "epp";
}

async function runInTerminal(command, cwd) {
  const term = vscode.window.createTerminal({ name: "E++", cwd });
  term.show(true);
  term.sendText(command, true);
}

async function withEppFile(uri, fn) {
  let doc = null;
  const isEppLikeFile = (filePath) =>
    !!filePath &&
    (filePath.toLowerCase().endsWith(".epp") ||
      path.basename(filePath).toLowerCase() === "__init__" ||
      path.basename(filePath).toLowerCase() === ".__init__");

  if (uri && uri.fsPath && isEppLikeFile(uri.fsPath)) {
    doc = await vscode.workspace.openTextDocument(uri);
  } else {
    const editor = vscode.window.activeTextEditor;
    if (editor && editor.document.languageId === "epp") doc = editor.document;
  }

  if (!doc || doc.languageId !== "epp") {
    vscode.window.showErrorMessage("Abre o selecciona un archivo E++ (.epp, __init__ o .__init__) para usar este comando.");
    return;
  }

  const cfg = vscode.workspace.getConfiguration("eppLanguage");
  if (cfg.get("autoSaveBeforeRun", true) && doc.isDirty) {
    await doc.save();
  }
  await fn(doc);
}

async function commandRunFile(uri) {
  await withEppFile(uri, async (doc) => {
    const workspaceRoot = getWorkspaceFolderForUri(doc.uri);
    const cliPath = detectCliPath(workspaceRoot);
    const cmd = `${quoteArg(cliPath)} run ${quoteArg(doc.fileName)}`;
    await runInTerminal(cmd, workspaceRoot);
  });
}

async function commandCheckFile(uri) {
  await withEppFile(uri, async (doc) => {
    const workspaceRoot = getWorkspaceFolderForUri(doc.uri);
    const cliPath = detectCliPath(workspaceRoot);
    const cmd = `${quoteArg(cliPath)} check ${quoteArg(doc.fileName)}`;
    await runInTerminal(cmd, workspaceRoot);
  });
}

async function commandCompileFile(uri) {
  await withEppFile(uri, async (doc) => {
    const workspaceRoot = getWorkspaceFolderForUri(doc.uri);
    const cliPath = detectCliPath(workspaceRoot);
    const cfg = vscode.workspace.getConfiguration("eppLanguage");
    const configuredOut = String(cfg.get("defaultCompileOutput", "") || "").trim();
    const output = configuredOut || path.join(path.dirname(doc.fileName), `${path.parse(doc.fileName).name}.exe`);
    const cmd = `${quoteArg(cliPath)} compile ${quoteArg(doc.fileName)} ${quoteArg(output)}`;
    await runInTerminal(cmd, workspaceRoot);
  });
}

async function commandDoctor() {
  const editor = vscode.window.activeTextEditor;
  const workspaceRoot = getWorkspaceFolderForUri(editor ? editor.document.uri : null);
  const cliPath = detectCliPath(workspaceRoot);
  const cmd = `${quoteArg(cliPath)} doctor`;
  await runInTerminal(cmd, workspaceRoot);
}

function collectImportModuleSuggestions(document) {
  const out = new Set();
  const workspaceRoot = getWorkspaceFolderForUri(document.uri);
  const dirs = [
    path.join(workspaceRoot, "examples", "libs", "stdlib"),
    path.join(workspaceRoot, "examples", "libs"),
    path.join(workspaceRoot, "libs"),
  ];

  for (const dir of dirs) {
    try {
      const entries = fs.readdirSync(dir, { withFileTypes: true });
      for (const ent of entries) {
        if (ent.isFile()) {
          if (ent.name.endsWith(".epp")) {
            const base = ent.name.slice(0, -4);
            if (base && base !== "__init__") out.add(base);
          } else if (ent.name === "__init__" || ent.name === ".__init__") {
            out.add(path.basename(dir));
          }
        } else if (ent.isDirectory()) {
          const initNoExt = path.join(dir, ent.name, "__init__");
          const initEpp = path.join(dir, ent.name, "__init__.epp");
          const dotInitNoExt = path.join(dir, ent.name, ".__init__");
          const dotInitEpp = path.join(dir, ent.name, ".__init__.epp");
          if (fs.existsSync(initNoExt) || fs.existsSync(initEpp) || fs.existsSync(dotInitNoExt) || fs.existsSync(dotInitEpp)) {
            out.add(ent.name);
          }
        }
      }
    } catch (_) {}
  }

  return Array.from(out).sort();
}

function collectDocumentSymbols(document) {
  return parseDocumentSemantic(document).symbols.map((s) => s.name);
}

function scanCodeLine(raw, state) {
  let inString = false;
  let out = "";
  let unclosedStringAt = -1;
  for (let i = 0; i < raw.length; i++) {
    const ch = raw[i];
    const next = i + 1 < raw.length ? raw[i + 1] : "";
    if (!inString && !state.inBlockComment && ch === "/" && next === "/") break;
    if (!inString && !state.inBlockComment && ch === "/" && next === "*") {
      state.inBlockComment = true;
      i++;
      continue;
    }
    if (!inString && state.inBlockComment && ch === "*" && next === "/") {
      state.inBlockComment = false;
      i++;
      continue;
    }
    if (state.inBlockComment) continue;

    if (ch === '"' && !isEscaped(raw, i)) {
      inString = !inString;
      out += " ";
      if (inString) unclosedStringAt = i;
      else unclosedStringAt = -1;
      continue;
    }
    if (inString) {
      out += " ";
      continue;
    }
    out += ch;
  }
  return { code: out, unclosedStringAt: inString ? unclosedStringAt : -1 };
}

function parseDocumentSemantic(document) {
  const key = document.uri.toString();
  const cached = parsedCache.get(key);
  if (cached && cached.version === document.version) return cached.data;

  const state = { inBlockComment: false };
  const symbols = [];
  const imports = [];
  const references = new Map();

  for (let line = 0; line < document.lineCount; line++) {
    const raw = document.lineAt(line).text;
    const { code } = scanCodeLine(raw, state);
    const parseModuleArg = (arg) => {
      if (/^"[^"]+"$/.test(arg)) return { moduleName: arg.slice(1, -1), quoted: true };
      if (/^[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*$/.test(arg)) return { moduleName: arg, quoted: false };
      return { moduleName: "", quoted: false };
    };

    const importMatch = code.match(/^\s*import\s+(.+?)\s*$/);
    if (importMatch) {
      const arg = importMatch[1].trim();
      const start = code.indexOf(arg);
      const end = start + arg.length;
      const parsedArg = parseModuleArg(arg);
      imports.push({
        moduleName: parsedArg.moduleName,
        quoted: parsedArg.quoted,
        arg,
        line,
        range: makeRange(line, start, end),
        kind: "import",
      });
    }

    const fromMatch = code.match(/^\s*from\s+(.+?)\s+import\s+(.+?)\s*$/);
    if (fromMatch) {
      const moduleArg = fromMatch[1].trim();
      const importedArg = fromMatch[2].trim();
      const moduleStart = code.indexOf(moduleArg);
      const parsedArg = parseModuleArg(moduleArg);
      const importedNames =
        importedArg === "*"
          ? ["*"]
          : importedArg
              .split(",")
              .map((s) => s.trim())
              .filter((s) => /^[A-Za-z_]\w*$/.test(s));
      imports.push({
        moduleName: parsedArg.moduleName,
        quoted: parsedArg.quoted,
        arg: moduleArg,
        line,
        range: makeRange(line, moduleStart, moduleStart + moduleArg.length),
        kind: "from",
        importedNames,
      });
    }

    const decls = [
      { re: /^\s*func\s+([A-Za-z_]\w*)/, kind: vscode.SymbolKind.Function, declType: "func" },
      { re: /^\s*class\s+([A-Za-z_]\w*)/, kind: vscode.SymbolKind.Class, declType: "class" },
      { re: /^\s*var\s+([A-Za-z_]\w*)/, kind: vscode.SymbolKind.Variable, declType: "var" },
    ];

    for (const decl of decls) {
      const m = code.match(decl.re);
      if (!m) continue;
      const name = m[1];
      const start = code.indexOf(name);
      symbols.push({
        name,
        kind: decl.kind,
        declType: decl.declType,
        line,
        range: makeRange(line, start, start + name.length),
      });
    }

    let token;
    SYMBOL_RE.lastIndex = 0;
    while ((token = SYMBOL_RE.exec(code)) !== null) {
      const name = token[0];
      const start = token.index;
      if (!references.has(name)) references.set(name, []);
      references.get(name).push(makeRange(line, start, start + name.length));
    }
  }

  const data = { symbols, imports, references };
  parsedCache.set(key, { version: document.version, data });
  return data;
}

async function resolveImportUri(document, imp) {
  if (!imp || !imp.moduleName) return null;
  const workspaceRoot = getWorkspaceFolderForUri(document.uri);
  const currentDir = path.dirname(document.fileName);
  const normalizeModulePath = (name, quoted) => {
    if (quoted) return name;
    return String(name).replace(/\./g, "/");
  };
  const buildModuleCandidates = (baseDir, modulePath) => {
    const out = [];
    const root = path.resolve(baseDir, modulePath);
    out.push(root);
    out.push(`${root}.epp`);
    out.push(path.join(root, "__init__"));
    out.push(path.join(root, "__init__.epp"));
    out.push(path.join(root, ".__init__"));
    out.push(path.join(root, ".__init__.epp"));
    return out;
  };

  const modulePath = normalizeModulePath(imp.moduleName, imp.quoted);
  const candidates = [];

  if (imp.quoted) {
    candidates.push(...buildModuleCandidates(currentDir, modulePath));
    candidates.push(...buildModuleCandidates(workspaceRoot, modulePath));
  } else {
    candidates.push(...buildModuleCandidates(currentDir, modulePath));
    candidates.push(...buildModuleCandidates(path.join(currentDir, "libs"), modulePath));
    candidates.push(...buildModuleCandidates(path.join(workspaceRoot, "libs"), modulePath));
    candidates.push(...buildModuleCandidates(path.join(workspaceRoot, "examples", "libs"), modulePath));
    candidates.push(...buildModuleCandidates(path.join(workspaceRoot, "examples", "libs", "stdlib"), modulePath));
  }
  for (const candidate of candidates) {
    if (fs.existsSync(candidate) && fs.statSync(candidate).isFile()) return vscode.Uri.file(candidate);
  }

  const fastPatterns = [
    `**/${modulePath}.epp`,
    `**/${modulePath}`,
    `**/${modulePath}/__init__`,
    `**/${modulePath}/__init__.epp`,
    `**/${modulePath}/.__init__`,
    `**/${modulePath}/.__init__.epp`,
  ];
  for (const pattern of fastPatterns) {
    const found = await vscode.workspace.findFiles(pattern, "**/{.git,build,node_modules}/**", 10);
    if (found.length > 0) return found[0];
  }
  return null;
}

async function collectImportedSymbolLocations(document, name) {
  const out = [];
  const parsed = parseDocumentSemantic(document);
  for (const imp of parsed.imports) {
    const uri = await resolveImportUri(document, imp);
    if (!uri) continue;
    const doc = await vscode.workspace.openTextDocument(uri);
    const parsedMod = parseDocumentSemantic(doc);
    for (const sym of parsedMod.symbols) {
      if (sym.name === name) out.push(new vscode.Location(uri, sym.range));
    }
  }
  return out;
}

function symbolItemKind(kind) {
  if (kind === vscode.SymbolKind.Function) return vscode.CompletionItemKind.Function;
  if (kind === vscode.SymbolKind.Class) return vscode.CompletionItemKind.Class;
  return vscode.CompletionItemKind.Variable;
}

async function validateDocument(document, collection) {
  if (document.languageId !== "epp") return;

  const diagnostics = [];
  const cfg = vscode.workspace.getConfiguration("eppLanguage");
  const styleWarnings = cfg.get("diagnostics.styleWarnings", false);
  const unresolvedImportWarnings = cfg.get("diagnostics.unresolvedImportWarnings", true);
  const braceStack = [];
  const parenStack = [];
  const state = { inBlockComment: false };
  const parsed = parseDocumentSemantic(document);

  for (let line = 0; line < document.lineCount; line++) {
    const raw = document.lineAt(line).text;
    const { code: codeLine, unclosedStringAt } = scanCodeLine(raw, state);
    const text = codeLine.trim();

    if (styleWarnings && /^\s*print\s+/.test(codeLine) && !/^\s*print\s*\(/.test(codeLine)) {
      const col = codeLine.indexOf("print");
      addDiagnostic(
        diagnostics,
        vscode.DiagnosticSeverity.Warning,
        line,
        col,
        col + 5,
        "Sintaxis antigua detectada: usa print(...).",
        "E-REC-PRINT"
      );
    }

    if (unclosedStringAt >= 0) {
      addDiagnostic(
        diagnostics,
        vscode.DiagnosticSeverity.Error,
        line,
        unclosedStringAt,
        raw.length,
        'Cadena sin cerrar. Falta comilla doble (").',
        "E-LEX-STRING"
      );
    }

    if (/^\s*import\s+$/.test(codeLine)) {
      const col = codeLine.indexOf("import");
      addDiagnostic(
        diagnostics,
        vscode.DiagnosticSeverity.Error,
        line,
        col,
        col + 6,
        "Import incompleto. Usa: import modulo, import paquete.modulo o import \"ruta/modulo.epp\"",
        "E-PARSE-IMPORT"
      );
    } else if (/^\s*import\s+/.test(codeLine)) {
      const m = codeLine.match(/^\s*import\s+(.+?)\s*$/);
      if (m) {
        const arg = m[1].trim();
        if (!/^"[^"]+"$/.test(arg) && !/^[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*$/.test(arg)) {
          const col = codeLine.indexOf(arg);
          addDiagnostic(
            diagnostics,
            vscode.DiagnosticSeverity.Error,
            line,
            col,
            col + arg.length,
            "Import invalido. Usa modulo simple o cadena entre comillas.",
            "E-PARSE-IMPORT"
          );
        }
      }
    } else if (/^\s*from\s+/.test(codeLine)) {
      const m = codeLine.match(/^\s*from\s+(.+?)\s+import\s+(.+?)\s*$/);
      if (!m) {
        const col = codeLine.indexOf("from");
        addDiagnostic(
          diagnostics,
          vscode.DiagnosticSeverity.Error,
          line,
          col >= 0 ? col : 0,
          col >= 0 ? col + 4 : 4,
          "Sintaxis invalida. Usa: from paquete.modulo import simbolo",
          "E-PARSE-FROM"
        );
      } else {
        const moduleArg = m[1].trim();
        const namesArg = m[2].trim();
        const moduleOk = /^"[^"]+"$/.test(moduleArg) || /^[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*$/.test(moduleArg);
        const namesOk =
          namesArg === "*" ||
          namesArg
            .split(",")
            .map((s) => s.trim())
            .filter((s) => s.length > 0)
            .every((s) => /^[A-Za-z_]\w*$/.test(s));
        if (!moduleOk || !namesOk) {
          const col = codeLine.indexOf(moduleArg);
          addDiagnostic(
            diagnostics,
            vscode.DiagnosticSeverity.Error,
            line,
            col >= 0 ? col : 0,
            codeLine.length,
            "Sintaxis invalida. Ejemplo: from stdlib.asyncio import sleep, gather",
            "E-PARSE-FROM"
          );
        }
      }
    }

    if (!text) continue;

    for (let i = 0; i < codeLine.length; i++) {
      const ch = codeLine[i];
      if (ch === "{") braceStack.push(new vscode.Position(line, i));
      if (ch === "}") {
        if (braceStack.length === 0) {
          addDiagnostic(
            diagnostics,
            vscode.DiagnosticSeverity.Error,
            line,
            i,
            i + 1,
            "Llave de cierre '}' sin apertura.",
            "E-PARSE-BRACE"
          );
        } else {
          braceStack.pop();
        }
      }

      if (ch === "(") parenStack.push(new vscode.Position(line, i));
      if (ch === ")") {
        if (parenStack.length === 0) {
          addDiagnostic(
            diagnostics,
            vscode.DiagnosticSeverity.Error,
            line,
            i,
            i + 1,
            "Parentesis ')' sin apertura.",
            "E-PARSE-PAREN"
          );
        } else {
          parenStack.pop();
        }
      }
    }
  }

  if (state.inBlockComment) {
    addDiagnostic(
      diagnostics,
      vscode.DiagnosticSeverity.Error,
      Math.max(0, document.lineCount - 1),
      0,
      2,
      "Comentario de bloque sin cerrar (*/).",
      "E-LEX-COMMENT"
    );
  }

  for (const pos of braceStack) {
    addDiagnostic(
      diagnostics,
      vscode.DiagnosticSeverity.Error,
      pos.line,
      pos.character,
      pos.character + 1,
      "Llave de apertura '{' sin cierre.",
      "E-PARSE-BRACE"
    );
  }

  for (const pos of parenStack) {
    addDiagnostic(
      diagnostics,
      vscode.DiagnosticSeverity.Error,
      pos.line,
      pos.character,
      pos.character + 1,
      "Parentesis de apertura '(' sin cierre.",
      "E-PARSE-PAREN"
    );
  }

  if (unresolvedImportWarnings) {
    for (const imp of parsed.imports) {
      if (!imp.moduleName) continue;
      const uri = await resolveImportUri(document, imp);
      if (!uri) {
        addDiagnostic(
          diagnostics,
          vscode.DiagnosticSeverity.Warning,
          imp.line,
          imp.range.start.character,
          imp.range.end.character,
          `No se encontro modulo '${imp.moduleName}'.`,
          "E-IMPORT-NOTFOUND"
        );
      }
    }
  }

  collection.set(document.uri, diagnostics);
}

function activate(context) {
  const diagnostics = vscode.languages.createDiagnosticCollection("epp");
  context.subscriptions.push(diagnostics);

  if (vscode.window.activeTextEditor) {
    validateDocument(vscode.window.activeTextEditor.document, diagnostics);
  }

  context.subscriptions.push(
    vscode.workspace.onDidOpenTextDocument((doc) => validateDocument(doc, diagnostics)),
    vscode.workspace.onDidChangeTextDocument((evt) => {
      parsedCache.delete(evt.document.uri.toString());
      validateDocument(evt.document, diagnostics);
    }),
    vscode.workspace.onDidCloseTextDocument((doc) => {
      parsedCache.delete(doc.uri.toString());
      diagnostics.delete(doc.uri);
    })
  );

  context.subscriptions.push(
    vscode.commands.registerCommand("eppLanguage.runFile", commandRunFile),
    vscode.commands.registerCommand("eppLanguage.checkFile", commandCheckFile),
    vscode.commands.registerCommand("eppLanguage.compileFile", commandCompileFile),
    vscode.commands.registerCommand("eppLanguage.doctor", commandDoctor)
  );

  const completionProvider = vscode.languages.registerCompletionItemProvider(
    { language: "epp" },
    {
      async provideCompletionItems(document, position) {
        const items = [];
        const seen = new Set();
        const parsed = parseDocumentSemantic(document);
        const linePrefix = document.lineAt(position.line).text.slice(0, position.character);
        const pushUnique = (item) => {
          const key = `${item.label}:${item.kind}`;
          if (seen.has(key)) return;
          seen.add(key);
          items.push(item);
        };

        const importContext = /^\s*import\s+["A-Za-z0-9_./-]*$/.test(linePrefix);
        const fromModuleContext = /^\s*from\s+["A-Za-z0-9_./-]*$/.test(linePrefix);
        if (importContext || fromModuleContext) {
          const modules = collectImportModuleSuggestions(document);
          for (const mod of modules) {
            const item = new vscode.CompletionItem(mod, vscode.CompletionItemKind.Module);
            item.detail = "Modulo E++";
            item.insertText = mod;
            pushUnique(item);
          }
        }

        const fromSymbolsContext = linePrefix.match(/^\s*from\s+(.+?)\s+import\s+([A-Za-z0-9_,\s*]*)$/);
        if (fromSymbolsContext) {
          const moduleArg = fromSymbolsContext[1].trim();
          const quoted = /^"[^"]+"$/.test(moduleArg);
          const moduleName = quoted ? moduleArg.slice(1, -1) : moduleArg;
          const uri = await resolveImportUri(document, { moduleName, quoted });
          if (uri) {
            const modDoc = await vscode.workspace.openTextDocument(uri);
            const modParsed = parseDocumentSemantic(modDoc);
            for (const sym of modParsed.symbols) {
              const item = new vscode.CompletionItem(sym.name, symbolItemKind(sym.kind));
              item.detail = `Desde ${path.basename(uri.fsPath)}`;
              item.insertText = sym.name;
              pushUnique(item);
            }
          }
        }

        for (const kw of KEYWORDS) {
          const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
          item.detail = "Keyword E++";
          pushUnique(item);
        }

        for (const [fn, meta] of Object.entries(BUILTIN_SIGNATURES)) {
          const item = new vscode.CompletionItem(fn, vscode.CompletionItemKind.Function);
          item.detail = meta.sig;
          item.documentation = meta.doc;
          item.insertText = new vscode.SnippetString(`${fn}($1)`);
          pushUnique(item);
        }

        for (const [fn, meta] of Object.entries(STDLIB_SIGNATURES)) {
          const kind = /^[A-Z]/.test(fn) ? vscode.CompletionItemKind.Class : vscode.CompletionItemKind.Function;
          const item = new vscode.CompletionItem(fn, kind);
          item.detail = meta.sig;
          item.documentation = meta.doc;
          item.insertText = new vscode.SnippetString(`${fn}($1)`);
          pushUnique(item);
        }

        for (const sym of parsed.symbols) {
          const item = new vscode.CompletionItem(sym.name, symbolItemKind(sym.kind));
          item.detail = "Simbolo local";
          pushUnique(item);
        }

        for (const imp of parsed.imports) {
          const uri = await resolveImportUri(document, imp);
          if (!uri) continue;
          const modDoc = await vscode.workspace.openTextDocument(uri);
          const modParsed = parseDocumentSemantic(modDoc);
          for (const sym of modParsed.symbols) {
            const item = new vscode.CompletionItem(sym.name, symbolItemKind(sym.kind));
            item.detail = `Desde ${path.basename(uri.fsPath)}`;
            pushUnique(item);
          }
        }

        return items;
      },
    },
    ".",
    '"',
    "/"
  );
  context.subscriptions.push(completionProvider);

  const signatureProvider = vscode.languages.registerSignatureHelpProvider(
    { language: "epp" },
    {
      provideSignatureHelp(document, position) {
        const line = document.lineAt(position.line).text.slice(0, position.character);
        const m = line.match(/([A-Za-z_]\w*)\s*\([^()]*$/);
        if (!m) return null;
        const fn = m[1];
        const meta = BUILTIN_SIGNATURES[fn] || STDLIB_SIGNATURES[fn];
        if (!meta) return null;

        const help = new vscode.SignatureHelp();
        const sig = new vscode.SignatureInformation(meta.sig, meta.doc);
        help.signatures = [sig];
        help.activeSignature = 0;
        help.activeParameter = 0;
        return help;
      },
    },
    "(",
    ","
  );
  context.subscriptions.push(signatureProvider);

  const hoverProvider = vscode.languages.registerHoverProvider({ language: "epp" }, {
    async provideHover(document, position) {
      const wordRange = document.getWordRangeAtPosition(position);
      if (!wordRange) return null;
      const word = document.getText(wordRange);
      const parsed = parseDocumentSemantic(document);

      if (BUILTIN_SIGNATURES[word]) {
        const meta = BUILTIN_SIGNATURES[word];
        return new vscode.Hover(`\`${meta.sig}\`\n\n${meta.doc}`);
      }
      if (STDLIB_SIGNATURES[word]) {
        const meta = STDLIB_SIGNATURES[word];
        return new vscode.Hover(`\`${meta.sig}\`\n\n${meta.doc}`);
      }
      if (word === "import") {
        return new vscode.Hover("Soporta: `import time`, `import stdlib.asyncio` o `import \"libs/modulo.epp\"`.");
      }
      if (word === "from") {
        return new vscode.Hover("Import parcial: `from stdlib.asyncio import sleep, gather`.");
      }
      if (word === "class") {
        return new vscode.Hover("Define clases con `init(...)`, `this` y metodos.");
      }

      const local = parsed.symbols.find((s) => s.name === word);
      if (local) {
        return new vscode.Hover(`\`${local.declType} ${word}\`\n\nDefinido en este archivo (linea ${local.line + 1}).`);
      }

      const imported = await collectImportedSymbolLocations(document, word);
      if (imported.length > 0) {
        return new vscode.Hover(`\`${word}\`\n\nDefinido en \`${path.basename(imported[0].uri.fsPath)}\`.`);
      }
      return null;
    }
  });
  context.subscriptions.push(hoverProvider);

  const definitionProvider = vscode.languages.registerDefinitionProvider({ language: "epp" }, {
    async provideDefinition(document, position) {
      const wordRange = document.getWordRangeAtPosition(position);
      if (!wordRange) return null;
      const word = document.getText(wordRange);
      const parsed = parseDocumentSemantic(document);

      for (const imp of parsed.imports) {
        if (imp.moduleName === word && imp.range.contains(position)) {
          const uri = await resolveImportUri(document, imp);
          if (uri) return new vscode.Location(uri, new vscode.Position(0, 0));
        }
      }

      const local = parsed.symbols
        .filter((s) => s.name === word)
        .map((s) => new vscode.Location(document.uri, s.range));
      if (local.length > 0) return local;

      const imported = await collectImportedSymbolLocations(document, word);
      if (imported.length > 0) return imported;
      return null;
    }
  });
  context.subscriptions.push(definitionProvider);

  const referencesProvider = vscode.languages.registerReferenceProvider({ language: "epp" }, {
    provideReferences(document, position, ctx) {
      const wordRange = document.getWordRangeAtPosition(position);
      if (!wordRange) return [];
      const word = document.getText(wordRange);
      const parsed = parseDocumentSemantic(document);
      const refs = parsed.references.get(word) || [];
      const out = [];
      for (const r of refs) {
        const isDecl = parsed.symbols.some((s) => s.name === word && s.range.isEqual(r));
        if (!ctx.includeDeclaration && isDecl) continue;
        out.push(new vscode.Location(document.uri, r));
      }
      return out;
    }
  });
  context.subscriptions.push(referencesProvider);

  const documentSymbolProvider = vscode.languages.registerDocumentSymbolProvider({ language: "epp" }, {
    provideDocumentSymbols(document) {
      const parsed = parseDocumentSemantic(document);
      return parsed.symbols.map((s) => new vscode.DocumentSymbol(s.name, s.declType, s.kind, s.range, s.range));
    }
  });
  context.subscriptions.push(documentSymbolProvider);

  const workspaceSymbolProvider = vscode.languages.registerWorkspaceSymbolProvider({
    async provideWorkspaceSymbols(query) {
      if (!query || query.trim().length < 2) return [];
      const files = await vscode.workspace.findFiles("**/*.epp", "**/{.git,build,node_modules}/**", 300);
      const initFiles = await vscode.workspace.findFiles("**/__init__", "**/{.git,build,node_modules}/**", 120);
      const dotInitFiles = await vscode.workspace.findFiles("**/.__init__", "**/{.git,build,node_modules}/**", 120);
      const merged = files.concat(initFiles).concat(dotInitFiles);
      const out = [];
      for (const uri of merged) {
        const doc = await vscode.workspace.openTextDocument(uri);
        const parsed = parseDocumentSemantic(doc);
        for (const s of parsed.symbols) {
          if (s.name.toLowerCase().includes(query.toLowerCase())) {
            out.push(new vscode.SymbolInformation(s.name, s.kind, "", new vscode.Location(uri, s.range)));
          }
        }
      }
      return out;
    }
  });
  context.subscriptions.push(workspaceSymbolProvider);

  const quickFixProvider = vscode.languages.registerCodeActionsProvider(
    { language: "epp" },
    {
      provideCodeActions(document, _range, contextActions) {
        const actions = [];
        for (const d of contextActions.diagnostics) {
          if (d.code === "E-PARSE-IMPORT") {
            const fix = new vscode.CodeAction("Corregir import de ejemplo", vscode.CodeActionKind.QuickFix);
            fix.diagnostics = [d];
            fix.edit = new vscode.WorkspaceEdit();
            fix.edit.replace(document.uri, d.range, "import \"libs/modulo.epp\"");
            actions.push(fix);
          } else if (d.code === "E-PARSE-FROM") {
            const fix = new vscode.CodeAction("Corregir from-import de ejemplo", vscode.CodeActionKind.QuickFix);
            fix.diagnostics = [d];
            fix.edit = new vscode.WorkspaceEdit();
            fix.edit.replace(document.uri, d.range, "from stdlib.asyncio import sleep");
            actions.push(fix);
          }
        }
        return actions;
      }
    },
    { providedCodeActionKinds: [vscode.CodeActionKind.QuickFix] }
  );
  context.subscriptions.push(quickFixProvider);
}

function deactivate() {}

module.exports = {
  activate,
  deactivate,
};
