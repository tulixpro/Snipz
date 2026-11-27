# RetroVault

RetroVault is a small German-language game library website and Electron desktop-launcher prototype.

## What works

- Responsive game library with search and category filters.
- Official download-page links for SuperTuxKart and OpenTTD.
- Electron desktop wrapper for Windows.
- External links are restricted to the official game websites listed in `main.cjs`.
- GitHub Actions workflow builds a Windows installer when this branch is updated.

## Important limits

- This is a prototype, not a released or signed installer.
- The launcher opens official download pages; it does not silently download, install, update, or launch game files.
- RetroVault does not host game files or redistribute third-party games.
- Only add games after checking their age rating, redistribution license, and official download source. Age-restricted titles are not included in this starter collection.
- Do not upload copyrighted game files, unofficial repacks, cracks, or modified installers.

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

Private/local prototype status is recommended until a parent or guardian has reviewed the project, the game list, and the publication settings. No website has been deployed by this repository change.
