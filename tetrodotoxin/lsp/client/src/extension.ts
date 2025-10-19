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

  // const serverPath = context.asAbsolutePath(
  //   path.join("..", "..", "..", ".bin", "bin", "tetrodotoxin", "lsp", "server", "ttx-lang-server")
  // );

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
      // Notify the server about file changes to '.clientrc files contained in the workspace
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

  // Semantic highlighting
  const tokenTypes = [
    'ttxId', // Raw Identifier
    'ttxP',  // Function parameter
    'ttxFu', // Function call
    'ttxMe', // Member access
    'ttxDT', // Definition Type
    'ttxDI', // Definition ID
    'ttxCm', // Comment
    'ttxDis', // Disabled
    'ttxIn', // imports
    'ttxNm', // Package Name
    'ttxM1', // Constant
    'ttxM2', // Dynamic
    'ttxM3', // Hidden
    'ttxM4', // Temporary

    'ttxS', // Strings
    'ttxI', // Ints
    'ttxN', // Numbers
    'ttxT', // Type
    'ttxA', // Attribute
    'ttxZ', // This opperator: aka $
    'ttxD', // Definition makers
    'ttxK', // Unified Keywords
    'ttxM', // Unified Modifiers
    'ttxO', // Unified Opperations
    'ttxC', // Unified Checks
    'ttxE', // End Statment: aka ;
    'ttxSS', // Scopes: aka { }
    'ttxSE', // Scopes: aka { }
    'ttxGS', // Groups: aka ( )
    'ttxGE', // Groups: aka ( )
    'ttxIS', // Indexs: aka [ ]
    'ttxIE', // Indexs: aka [ ]
    'ttxL', // Logical controls for the compiler.
    'ttx_', // Seperator

    // No semantic override
    'cppId', // Raw Identifier
    'cppP',  // Function parameter
    'cppFu', // Function call
    'cppMe', // Member access
    'cppDT', // Definition Type
    'cppDI', // Definition ID
    'cppCm', // Comment
    'cppDis', // Disabled
    'cppIn', // imports
    'cppNm', // Package Name
    'cppM1', // Constant
    'cppM2', // Dynamic
    'cppM3', // Hidden
    'cppM4', // Temporary

    'cppS', // Strings
    'cppI', // Ints
    'cppN', // Numbers
    'cppT', // Type
    'cppA', // Attribute
    'cppZ', // This opperator: aka $
    'cppD', // Definition makers
    'cppK', // Unified Keywords
    'cppM', // Unified Modifiers
    'cppO', // Unified Opperations
    'cppC', // Unified Checks
    'cppE', // End Statment: aka ;
    'cppSS', // Scopes: aka { }
    'cppSE', // Scopes: aka { }
    'cppGS', // Groups: aka ( )
    'cppGE', // Groups: aka ( )
    'cppIS', // Indexs: aka [ ]
    'cppIE', // Indexs: aka [ ]
    'cppL', // Logical controls for the compiler.
    'cpp_', // Seperator
  ];
  const tokenModifiers = ['declaration', 'documentation'];
  const legend = new vscode.SemanticTokensLegend(tokenTypes, tokenModifiers);

  const semantic_provider: vscode.DocumentSemanticTokensProvider = {
    provideDocumentSemanticTokens(document: vscode.TextDocument): vscode.ProviderResult<vscode.SemanticTokens> {
      return new Promise<vscode.SemanticTokens>(async (resolve) => {
        // analyze the document and return semantic tokens
        class TtxTokens {
          color: boolean;
          tokens: Array<Array<any>>;
        };

        const result = await client.sendRequest("tokenize", {
          "source": document.getText()
        }) as TtxTokens;

        const tokensBuilder = new vscode.SemanticTokensBuilder(legend);
        const typeHeader = result.color ? 'ttx' : 'cpp';
        ttxChannel.appendLine(`Lsp header: ${typeHeader}`);
        result.tokens.forEach((token) => {
          if (!tokenTypes.includes(typeHeader + token[0])) {
            ttxChannel.appendLine(`unknown Lsp type: ${typeHeader}${token[0]}`);
            return;
          }

          // VSCode doesn't support multi column tokens.
          if (token[1] != token[3]) {
            // Start Line
            tokensBuilder.push(
              new vscode.Range(
                new vscode.Position(token[1], token[2]),
                new vscode.Position(token[1], document.lineAt(token[1]).range.end.character)),
              typeHeader + token[0]
            );

            // Middle Lines (Completely covered)
            for (var i = token[1] + 1; i < token[3]; i++) {
              tokensBuilder.push(
                new vscode.Range(
                  new vscode.Position(i, 0),
                  new vscode.Position(i, document.lineAt(i).range.end.character)),
                typeHeader + token[0]
              );
            }

            // End Line
            tokensBuilder.push(
              new vscode.Range(
                new vscode.Position(token[3], 0),
                new vscode.Position(token[3], token[4])),
              typeHeader + token[0]
            );
          } else {
            tokensBuilder.push(
              new vscode.Range(
                new vscode.Position(token[1], token[2]),
                new vscode.Position(token[3], token[4])),
              typeHeader + token[0]
            );
          }
        });

        resolve(tokensBuilder.build());
      });
    }
  };

  let syntax_client = vscode.languages.registerDocumentSemanticTokensProvider(lang_selector, semantic_provider, legend);
  context.subscriptions.push(syntax_client);

  vscode.languages.registerDocumentFormattingEditProvider(lang_selector, {
    provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.ProviderResult<vscode.TextEdit[]> {
      return new Promise<vscode.TextEdit[]>(async (resolve) => {
        // analyze the document and return semantic tokens
        class TtxFormat {
          document: string
        };

        const result = await client.sendRequest("format", {
          "source": document.getText(),
          "name": document.fileName,
        }) as TtxFormat;

        var firstLine = document.lineAt(0);
        var lastLine = document.lineAt(document.lineCount - 1);
        var textRange = new vscode.Range(firstLine.range.start, lastLine.range.end);
        resolve([vscode.TextEdit.replace(textRange, result.document)]);
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