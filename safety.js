/* Snipz Safety Center — UI prototype only; server-side enforcement is not connected. */
(() => {
  const view = document.getElementById('snipzSocialView');
  const tabs = document.getElementById('snipzTabs');
  const content = document.getElementById('snipzSocialContent');
  if (!view || !tabs || !content || document.getElementById('snipzSafetyTab')) return;

  const demoPeople = [
    { id: 'mila', name: 'Mila' },
    { id: 'noah', name: 'Noah' },
    { id: 'lea', name: 'Lea' },
    { id: 'emil', name: 'Emil' }
  ];
  const blocked = new Set();
  const reports = [];
  const tab = document.createElement('button');
  tab.type = 'button';
  tab.id = 'snipzSafetyTab';
  tab.className = 'social-tab';
  tab.dataset.safetyTab = 'true';
  tab.textContent = '🛡️ Sicherheit';
  tabs.appendChild(tab);

  const esc = value => String(value).replace(/[&<>"']/g, c => ({
    '&':'&amp;', '<':'&lt;', '>':'&gt;', '"':'&quot;', "'":'&#39;'
  })[c]);

  function render() {
    tabs.querySelectorAll('.social-tab').forEach(button => {
      button.classList.toggle('active', button === tab);
    });
    content.innerHTML = `
      <section class="social-card safety-card" aria-labelledby="snipzSafetyTitle">
        <div class="safety-kicker">DEIN SCHUTZ</div>
        <h3 id="snipzSafetyTitle">Sicherheitscenter</h3>
        <p>Hier findest du wichtige Schutzfunktionen. Snipz ist an dieser Stelle noch eine Vorschau: Sperren und Meldungen werden nicht an einen Server übertragen.</p>
        <div class="safety-grid">
          <article class="safety-panel">
            <h4>🛑 Demo-Konten blockieren</h4>
            <p>Ein Blockieren in dieser Vorschau wirkt nur bis zum Neuladen und schützt noch keine echten Konten.</p>
            <div class="safety-people">${demoPeople.map(person => `
              <div class="safety-person">
                <span>${esc(person.name)}</span>
                <button type="button" class="social-action" data-safety-block="${person.id}">${blocked.has(person.id) ? 'Entsperren' : 'Blockieren'}</button>
              </div>`).join('')}</div>
          </article>
          <article class="safety-panel">
            <h4>🚩 Inhalt oder Person melden</h4>
            <form id="snipzSafetyReport" class="safety-report">
              <label for="snipzReportType">Was möchtest du melden?</label>
              <select id="snipzReportType" required>
                <option value="">Bitte auswählen</option>
                <option>Belästigung oder Mobbing</option>
                <option>Bedrohung oder gefährlicher Inhalt</option>
                <option>Unangemessener Inhalt</option>
                <option>Urheberrecht / fremde Inhalte</option>
                <option>Spam oder Betrug</option>
                <option>Sonstiges</option>
              </select>
              <label for="snipzReportDetails">Kurze Beschreibung (keine privaten Daten eingeben)</label>
              <textarea id="snipzReportDetails" maxlength="500" rows="3" placeholder="Was ist passiert?"></textarea>
              <button class="social-action primary-action" type="submit">Meldung als Demo speichern</button>
            </form>
            <p class="safety-hint" id="snipzReportStatus" role="status">Demo-Meldungen bleiben nur im Arbeitsspeicher dieser Seite.</p>
          </article>
        </div>
        <div class="safety-panel safety-wide">
          <h4>🔒 Datenschutz & Kontosicherheit</h4>
          <ul>
            <li>Teile niemals Passwort, Login-Code, Adresse, Schule oder Live-Standort.</li>
            <li>Erlaube Kamera und Mikrofon nur, wenn du die Funktion gerade verwendest.</li>
            <li>Veröffentliche keine Bilder oder Videos anderer Personen ohne deren Zustimmung.</li>
            <li>Snaps können fotografiert oder anderweitig gespeichert werden – Vertrauliches gehört nicht hinein.</li>
            <li>Nutze ein eigenes, starkes Passwort und gib es niemandem weiter.</li>
            <li>Wenn dich jemand bedrängt oder bedroht: antworte nicht weiter, sichere Belege und wende dich an eine vertraute erwachsene Person.</li>
          </ul>
        </div>
        <div class="safety-status"><strong>Aktueller Stand:</strong> Schutzoberfläche im Demo-Modus. Echte Kontosperren, Meldungsprüfung, Zugriffskontrolle, sichere Medienablage und Server-Löschung sind noch nicht angeschlossen.</div>
      </section>`;
  }

  tabs.addEventListener('click', event => {
    if (event.target.closest('[data-safety-tab]')) render();
  });
  content.addEventListener('click', event => {
    const button = event.target.closest('[data-safety-block]');
    if (!button) return;
    const id = button.dataset.safetyBlock;
    if (blocked.has(id)) blocked.delete(id); else blocked.add(id);
    render();
  });
  content.addEventListener('submit', event => {
    if (event.target.id !== 'snipzSafetyReport') return;
    event.preventDefault();
    const type = document.getElementById('snipzReportType').value;
    const details = document.getElementById('snipzReportDetails').value.trim();
    if (!type) return;
    reports.push({ type, details, createdAt: Date.now() });
    const status = document.getElementById('snipzReportStatus');
    if (status) status.textContent = 'Demo-Meldung lokal vorgemerkt (' + reports.length + '). Sie wurde NICHT versendet oder geprüft.';
    event.target.reset();
  });
})();