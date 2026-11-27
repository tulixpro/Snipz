# RetroVault

RetroVault is a German-language game library website and Electron desktop-launcher prototype.

## What works

- Responsive game library with search and category filters.
- Five free/open-source game entries with links to their official project download pages.
- Emulator directory linking to official DuckStation, PCSX2, PPSSPP, RPCS3, Xenia and RetroArch project sites.
- Browser Gamepad API support for basic page navigation (D-pad/stick, A/Cross to activate, B/Circle to go back to the top).
- Electron desktop wrapper for Windows.
- External links from the desktop app are restricted to the approved HTTPS host list in `main.cjs`.
- GitHub Actions workflow builds a Windows installer when this branch is updated.

## Important limits

- This is a prototype, not a released or signed installer.
- The launcher opens official project/download pages; it does not install emulators, games, BIOS, firmware or keys automatically.
- Gamepad navigation controls the RetroVault interface. Each emulator must separately support and be configured for a controller.
- Xbox 360 emulation through Xenia is experimental and compatibility varies. RetroVault does not advertise an Xbox One emulator.
- RetroVault does not host game files or redistribute third-party games.
- Only add games after checking their age rating, redistribution license, and official download source.
- Do not upload copyrighted game files, unofficial repacks, cracks, BIOS, firmware or encryption keys.

## Run locally

Requires Node.js 22 or later.

```bash
cd arcade-launcher
npm install
npm start
```

## Build the Windows installer

```bash
npm run dist
```

The installer is written to `dist/`. The GitHub Actions workflow also builds it and stores it as a temporary workflow artifact.

## Project status

This is a prototype. A parent or guardian should review the project, the game list, and publication settings before public release. No website has been deployed by this repository change.
