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

let client: LanguageClient | undefined;
let ttx_channel: OutputChannel | undefined;

const semantic_highlighting_setting = "semanticHighlighting.enabled";

// TextMate remains the default color owner. The semantic middleware suppresses
// token requests until the setting explicitly opts the document into them.
function semantic_highlighting_enabled(
  language_id: string,
  document?: vscode.TextDocument
): boolean {
  return workspace
    .getConfiguration(language_id, document?.uri)
    .get<boolean>(semantic_highlighting_setting, false);
}

export function start_language_client(
  context: ExtensionContext,
  language_id: string
): void {
  // One client owns the packaged Puffer process, diagnostics, hover, tokens,
  // and formatting for the extension lifetime.
  const language_selector = { language: language_id, scheme: "file" };
  ttx_channel = window.createOutputChannel("TTX Language Server");
  context.subscriptions.push(ttx_channel);

  const server_path = context.asAbsolutePath(path.join(".", "puffer"));
  const packages_root = context.asAbsolutePath("packages");
  ttx_channel.appendLine(`Launching Puffer LSP using path: ${server_path}`);

  const server_options: ServerOptions = {
    run: {
      command: server_path,
      args: [`-packages-root=${packages_root}`],
      transport: TransportKind.pipe,
    },
    debug: {
      command: server_path,
      args: [`-packages-root=${packages_root}`],
      transport: TransportKind.pipe,
    },
  };

  const client_options: LanguageClientOptions = {
    documentSelector: [language_selector],
    outputChannel: ttx_channel,
    middleware: {
      handleDiagnostics: (uri, diagnostics, next) => {
        ttx_channel?.appendLine(`Received diagnostics for ${uri}:`);
        diagnostics.forEach((diagnostic) =>
          ttx_channel?.appendLine(`  ${diagnostic.message}`)
        );
        return next(uri, diagnostics);
      },
      provideDocumentSemanticTokens: (document, token, next) => {
        if (!semantic_highlighting_enabled(language_id, document)) {
          return null;
        }
        return next(document, token);
      },
      provideDocumentSemanticTokensEdits: (
        document,
        previous_result_id,
        token,
        next
      ) => {
        if (!semantic_highlighting_enabled(language_id, document)) {
          return null;
        }
        return next(document, previous_result_id, token);
      },
      provideDocumentRangeSemanticTokens: (document, range, token, next) => {
        if (!semantic_highlighting_enabled(language_id, document)) {
          return null;
        }
        return next(document, range, token);
      },
    },
    synchronize: {
      fileEvents: workspace.createFileSystemWatcher("**/*.ttx"),
    },
  };

  const language_client = new LanguageClient(
    "TetrodotoxinLanguageServer",
    "TTX Language Server",
    server_options,
    client_options
  );
  client = language_client;

  language_client.onDidChangeState((event) => {
    ttx_channel?.appendLine(
      `Client state changed: ${event.oldState} -> ${event.newState}`
    );
  });

  language_client.start();
  ttx_channel.appendLine("LSP client started.");
  ttx_channel.appendLine(
    `Semantic highlighting: ${
      semantic_highlighting_enabled(language_id) ? "enabled" : "disabled"
    }`
  );

  context.subscriptions.push(
    workspace.onDidChangeConfiguration((event) => {
      if (
        event.affectsConfiguration(
          `${language_id}.${semantic_highlighting_setting}`
        )
      ) {
        ttx_channel?.appendLine(
          `Semantic highlighting: ${
            semantic_highlighting_enabled(language_id)
              ? "enabled"
              : "disabled"
          }`
        );
        void vscode.commands
          .executeCommand("editor.action.restartSemanticTokens")
          .then(undefined, () => undefined);
      }
    })
  );

}

export function deactivate_language_client(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  ttx_channel?.appendLine("LSP client stopped.");
  return client.stop();
}
