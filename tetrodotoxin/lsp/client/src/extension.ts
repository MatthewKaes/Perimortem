// Perimortem Engine
// Copyright © Matt Kaes

import * as path from "path";
import * as vscode from "vscode";
import { ExtensionContext, OutputChannel, window, workspace } from "vscode";
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  TransportKind,
} from "vscode-languageclient/node";

let client: LanguageClient;
let ttxChannel: OutputChannel;

export function activate(context: ExtensionContext) {
  const lang_selector = { language: 'tetrodotoxin', scheme: 'file' };
  ttxChannel = window.createOutputChannel("TTX Language Server");

  // Copy binary for publishing.
  const serverPath = context.asAbsolutePath(
    path.join(".", "ttx-lang-server")
  );

  ttxChannel.appendLine(`Launching Lsp Server using path: ${serverPath}`);

  const serverOptions: ServerOptions = {
    run: {
      command: serverPath,
      transport: TransportKind.pipe,
    },
    debug: {
      command: serverPath,
      transport: TransportKind.pipe,
    },
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector: [lang_selector],
    outputChannel: ttxChannel,
    middleware: {
      handleDiagnostics: (uri, diagnostics, next) => {
        ttxChannel.appendLine(`Received diagnostics for ${uri}:`);
        diagnostics.forEach((d) => ttxChannel.appendLine(`  ${d.message}`));
        return next(uri, diagnostics);
      },
    },
    synchronize: {
      fileEvents: workspace.createFileSystemWatcher('**/*.ttx')
    }
  };

  client = new LanguageClient(
    "TetrodotoxinLanguageServer",
    "TTX Language Server",
    serverOptions,
    clientOptions
  );

  client.onDidChangeState((e) => {
    ttxChannel.appendLine(
      `Client state changed: ${e.oldState} -> ${e.newState}`
    );
  });

  client.start();
  ttxChannel.appendLine("Lsp client started.");

  vscode.languages.registerDocumentFormattingEditProvider(lang_selector, {
    provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.ProviderResult<vscode.TextEdit[]> {
      return new Promise<vscode.TextEdit[]>(async (resolve) => {
        class TtxFormat {
          document: string
        };

        var buffer = Buffer.from(document.getText());
        var encoded_source = buffer.toString('base64');

        const result = await client.sendRequest("format", {
          "source": encoded_source,
          "name": document.fileName,
        }) as TtxFormat;

        var decoded_document = Buffer.from(result.document, 'base64').toString('utf8');
        var firstLine = document.lineAt(0);
        var lastLine = document.lineAt(document.lineCount - 1);
        var textRange = new vscode.Range(firstLine.range.start, lastLine.range.end);
        resolve([vscode.TextEdit.replace(textRange, decoded_document)]);
      });
    }
  });
}

export function deactivate(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  ttxChannel.appendLine("Lsp client stopped.");
  return client.stop();
}
