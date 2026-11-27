const { app, BrowserWindow, shell } = require("electron");
const path = require("node:path");

const allowedHosts = new Set([
  "supertuxkart.github.io",
  "www.openttd.org",
  "openttd.org"
]);

function openAllowedExternal(rawUrl) {
  try {
    const url = new URL(rawUrl);
    if (url.protocol !== "https:" || !allowedHosts.has(url.hostname)) return;
    return shell.openExternal(url.toString());
  } catch {
    return;
  }
}

function createWindow() {
  const win = new BrowserWindow({
    width: 1180,
    height: 820,
    minWidth: 360,
    minHeight: 600,
    backgroundColor: "#080d1b",
    title: "RetroVault",
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: true
    }
  });

  win.webContents.setWindowOpenHandler(({ url }) => {
    void openAllowedExternal(url);
    return { action: "deny" };
  });
  win.loadFile(path.join(__dirname, "index.html"));
}

app.whenReady().then(() => {
  createWindow();
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});
