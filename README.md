# Snipz

Snipz ist eine Kurzvideo-Plattform mit vertikalem Feed, Creator-Profilen, Likes, Kommentaren, Suche und Upload-Oberfläche.

## Aktueller Stand

- Die Frontend-Oberfläche wird auf dem Branch `design/blue-dark` vorbereitet.
- Enthalten sind das responsive Blue-Dark-Design, Video-Feed-Oberfläche, Suchfeld, Creator-/Profiloberfläche und ein kurzes Onboarding-Tutorial.
- Eine Web-App-Manifestdatei und ein SVG-Favicon ergänzen die Website-Grundausstattung.
- **Der Server ist absichtlich noch nicht fertig und wurde für diese Frontend-Arbeiten nicht verändert.**
- Konten, Feed-Daten, Uploads, Likes, Kommentare und Folgen funktionieren erst nach Einrichtung und Sicherheitstest des Backends.

## Lokaler Start der Oberfläche

Für die reine Weboberfläche kann ein statischer Server verwendet werden:

```bash
python -m http.server 8000
```

Dann http://localhost:8000 öffnen.

## Server später einrichten

Die Website enthält derzeit noch eine Supabase-Frontend-Anbindung mit Platzhaltern in `config.js`. Die Cloudflare-Dateien sind ein separates, noch unvollständiges Backend-Grundgerüst. Vor der Serverarbeit muss entschieden werden, ob Supabase weiterverwendet oder die Oberfläche vollständig auf die Cloudflare-API umgestellt wird. Beide Backends nicht gleichzeitig unkoordiniert aktivieren.

Siehe [cloudflare/README.md](cloudflare/README.md) für den aktuellen Cloudflare-Plan.

## Vor dem öffentlichen Start

- [ ] Backend-Architektur festlegen (Supabase oder Cloudflare)
- [ ] Anmeldung und Berechtigungen serverseitig absichern
- [ ] Uploads mit Größen-, Typ-, Quoten- und Viren-/Missbrauchsprüfungen absichern
- [ ] Feed, Suche, Likes, Kommentare und Folgen mit echten API-Daten verbinden
- [ ] Moderation, Meldefunktion und Admin-Berechtigungen testen
- [ ] Datenschutz, Kontolöschung und Datenexport ergänzen
- [ ] Backups, Monitoring und Wiederherstellung testen
- [ ] Browser- und Smartphone-Tests durchführen

## Sicherheit

Die aktuelle Sicherheits-Baseline und die noch offenen Anforderungen stehen in [SECURITY.md](./SECURITY.md). Der Worker ist absichtlich auf den Health-Check beschränkt, bis Authentifizierung und serverseitige Berechtigungen implementiert und getestet sind.


- Keine Datenbankpasswörter, Service-Role-Schlüssel oder privaten Tokens im Browser oder Repository.
- Admin-Rechte niemals aus Browser-Eingaben übernehmen.
- Uploads nur über serverseitig autorisierte, kurzlebige Freigaben.
- Moderation, Meldungen, Audit-Protokolle und Kontosperren müssen serverseitig umgesetzt werden.
