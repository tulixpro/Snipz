/* Snipz social frontend prototype. All interactions here use demo data only. */
(() => {
  const $ = id => document.getElementById(id);
  const main = document.querySelector('.layout main');
  if (!main || $('snipzSocialView')) return;
  const demoFriends = [
    {id:'mila',name:'Mila',handle:'@mila.blue',status:'Gerade aktiv',letter:'M'},
    {id:'noah',name:'Noah',handle:'@noahx',status:'Vor 8 Min. aktiv',letter:'N'},
    {id:'lea',name:'Lea',handle:'@lea.vibes',status:'Heute aktiv',letter:'L'},
    {id:'emil',name:'Emil',handle:'@emil_07',status:'Vor 1 Std. aktiv',letter:'E'}
  ];
  let friends = new Set(['mila','noah','lea']);
  let requests = [{id:'emil',name:'Emil',handle:'@emil_07',letter:'E'}];
  let selectedChat = 'mila';
  let messages = {
    mila:[{mine:false,text:'Hey! Willkommen bei Snipz 👋'},{mine:true,text:'Danke! Was geht?'}],
    noah:[{mine:false,text:'Hast du das neue Video gesehen?'}],
    lea:[{mine:false,text:'Schick mir später einen Snap ✨'}]
  };
  let activeTab='friends', currentFilter='none', localMediaUrl=null, cameraStream=null;
  const view=document.createElement('section');
  view.id='snipzSocialView';
  view.innerHTML = `
    <div class="social-head"><div><div class="social-muted">DEINE COMMUNITY</div><h2>Freunde & Nachrichten</h2></div><span class="social-muted">Snipz Social · Demo</span></div>
    <div class="social-card"><h3>Deine Stories</h3><div class="story-row" id="snipzStories"></div></div>
    <div class="social-tabs" id="snipzTabs">
      <button class="social-tab active" data-tab="friends">👥 Freunde</button>
      <button class="social-tab" data-tab="chats">💬 Chats</button>
      <button class="social-tab" data-tab="snaps">⚡ Snaps</button>
      <button class="social-tab" data-tab="camera">📸 Kamera & Filter</button>
    </div>
    <div id="snipzSocialContent"></div>
  `;
  main.insertBefore(view, main.firstChild);
  const content=$('snipzSocialContent');
  const nav=document.querySelector('.side');
  const socialNav=document.createElement('div');
  socialNav.className='social-nav';
  socialNav.innerHTML='<div class="nav" data-open-social="friends">👥 <span class="label">Freunde</span></div><div class="nav" data-open-social="chats">💬 <span class="label">Chats</span></div><div class="nav" data-open-social="snaps">⚡ <span class="label">Snaps</span></div><div class="nav" data-open-social="camera">📸 <span class="label">Kamera & Filter</span></div>';
  if(nav) nav.appendChild(socialNav);
  const mobile=document.querySelector('.mobile');
  if(mobile){
    const b=document.createElement('button');b.type='button';b.textContent='💬';b.setAttribute('aria-label','Freunde und Chats');b.dataset.openSocial='chats';mobile.insertBefore(b,mobile.lastElementChild);
  }
  function showSocial(tab){
    if(activeTab==='camera'&&tab!=='camera'&&cameraStream){cameraStream.getTracks().forEach(t=>t.stop());cameraStream=null;}
    activeTab=tab;view.classList.add('active');view.style.display='block';
    const feed=$('feed'),head=$('feedhead');
    if(feed)feed.style.display='none';if(head)head.style.display='none';
    document.querySelectorAll('[data-open-social]').forEach(x=>x.classList.toggle('active',x.dataset.openSocial===tab));
    document.querySelectorAll('[data-tab]').forEach(x=>x.classList.toggle('active',x.dataset.tab===tab));
    render();
  }
  function showFeed(){
    view.classList.remove('active');view.style.display='none';
    const feed=$('feed'),head=$('feedhead');
    if(feed)feed.style.display='';if(head)head.style.display='';
    document.querySelectorAll('[data-open-social]').forEach(x=>x.classList.remove('active'));
  }
  function avatar(letter){return '<div class="social-avatar">'+letter+'</div>'}
  function renderStories(){
    $('snipzStories').innerHTML='<button class="story" data-tab="camera"><div class="story-ring"><span>＋</span></div><small>Deine Story</small></button>'+demoFriends.filter(f=>friends.has(f.id)).map(f=>'<button class="story" data-story="'+f.id+'"><div class="story-ring"><span>'+f.letter+'</span></div><small>'+f.name+'</small></button>').join('');
  }
  function renderFriends(){
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Deine Freunde · '+friends.size+'</h3>'+demoFriends.filter(f=>friends.has(f.id)).map(f=>'<div class="friend-row">'+avatar(f.letter)+'<div class="social-grow"><b>'+f.name+'</b><small>'+f.handle+' · '+f.status+'</small></div><button class="social-action" data-chat="'+f.id+'">Chat</button></div>').join('')+'</div><div class="social-card"><h3>Freundschaftsanfragen</h3>'+(requests.length?requests.map(f=>'<div class="friend-row">'+avatar(f.letter)+'<div class="social-grow"><b>'+f.name+'</b><small>'+f.handle+'</small></div><button class="social-action primary-action" data-accept="'+f.id+'">Annehmen</button></div>').join(''):'<div class="social-empty">Keine neuen Anfragen</div>')+'<h3 style="margin-top:18px">Freunde entdecken</h3>'+demoFriends.filter(f=>!friends.has(f.id)&&!requests.some(r=>r.id===f.id)).map(f=>'<div class="friend-row">'+avatar(f.letter)+'<div class="social-grow"><b>'+f.name+'</b><small>'+f.handle+'</small></div><button class="social-action" data-add="'+f.id+'">Hinzufügen</button></div>').join('')+'</div></div><p class="social-note">Dies ist eine Vorschau mit erfundenen Profilen. Echte Kontakte und Anfragen kommen später mit dem Server.</p>';
  }
  function renderChats(){
    const f=demoFriends.find(x=>x.id===selectedChat)||demoFriends[0];
    const list=demoFriends.filter(x=>friends.has(x.id)).map(x=>'<button class="chat-row" style="width:100%;text-align:left;background:none;color:inherit;border:0" data-select-chat="'+x.id+'">'+avatar(x.letter)+'<span class="social-grow"><b>'+x.name+'</b><small>'+(messages[x.id]?.at(-1)?.text||'Noch keine Nachrichten')+'</small></span><span class="social-muted">›</span></button>').join('');
    const bubbles=(messages[selectedChat]||[]).map(m=>'<div class="chat-bubble '+(m.mine?'mine':'')+'">'+escapeHtml(m.text)+'</div>').join('');
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Nachrichten</h3>'+list+'</div><div class="social-card"><h3>Chat mit '+f.name+'</h3><div class="chat-window" id="snipzChatWindow">'+(bubbles||'<div class="social-empty">Schreib die erste Nachricht!</div>')+'</div><form class="chat-compose" id="snipzChatForm"><input id="snipzMessage" maxlength="500" placeholder="Nachricht schreiben …" autocomplete="off" required><button class="social-action primary-action">Senden</button></form></div></div><p class="social-note">Nachrichten werden aktuell nur in dieser Vorschau gehalten und nicht wirklich verschickt.</p>';
    const win=$('snipzChatWindow');if(win)win.scrollTop=win.scrollHeight;
  }
  function renderSnaps(){
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Neue Snaps</h3><div class="snap-row">'+avatar('M')+'<div class="social-grow"><b>Mila</b><small>Neuer Demo-Snap · vor 2 Min.</small></div><button class="social-action primary-action" data-open-snap="mila">Öffnen</button></div><div class="snap-row">'+avatar('L')+'<div class="social-grow"><b>Lea</b><small>Angesehener Demo-Snap</small></div><span class="social-muted">✓</span></div><div class="snap-row">'+avatar('N')+'<div class="social-grow"><b>Noah</b><small>Neuer Demo-Snap · vor 12 Min.</small></div><button class="social-action primary-action" data-open-snap="noah">Öffnen</button></div></div><div class="social-card"><h3>Deinen Snap erstellen</h3><div class="snap-preview" id="snipzSnapPreview">⚡<br>Dein Moment. Dein Snipz.</div><input class="social-input" style="margin-top:12px;width:100%" id="snipzSnapFile" type="file" accept="image/*,video/*"><button class="social-action primary-action" id="snipzSnapSend" style="margin-top:10px;width:100%">Snap vorbereiten</button><p class="social-note">Die Datei bleibt auf deinem Gerät. Ohne Server wird sie noch nicht an Freunde gesendet.</p></div></div>';
  }
  function renderCamera(){
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Kamera-Vorschau</h3><video id="snipzCameraVideo" class="camera-preview" autoplay muted playsinline></video><div class="snap-preview" id="snipzCameraPlaceholder">📸<br>Deine Kamera-Vorschau</div><div class="filter-row"><button class="filter-chip active" data-filter="none">Original</button><button class="filter-chip" data-filter="cool">Cool</button><button class="filter-chip" data-filter="warm">Warm</button><button class="filter-chip" data-filter="mono">S/W</button><button class="filter-chip" data-filter="dream">Dream</button></div><button class="social-action primary-action" id="snipzStartCamera">Kamera starten</button><p class="social-note">Die Kamera wird erst nach deinem Klick und deiner Browser-Erlaubnis aktiviert.</p></div><div class="social-card"><h3>Filter ausprobieren</h3><div class="snap-preview" id="snipzFilterSample">✨ Snipz vibes ✨</div><p class="social-note">Die Filter sind ein erster Demo-Entwurf. Später können wir Sticker, Text und weitere Kameraeffekte ergänzen.</p></div></div>';
    applyFilter();
  }
  function escapeHtml(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
  function applyFilter(){
    const video=$('snipzCameraVideo');if(video)video.className='camera-preview'+(currentFilter==='none'?'':' filter-'+currentFilter);
    const sample=$('snipzFilterSample');if(sample){sample.style.filter=({none:'none',cool:'saturate(1.3) hue-rotate(12deg) contrast(1.08)',warm:'sepia(.25) saturate(1.3)',mono:'grayscale(1) contrast(1.1)',dream:'hue-rotate(35deg) saturate(1.7) brightness(1.08)'})[currentFilter];}
    document.querySelectorAll('[data-filter]').forEach(b=>b.classList.toggle('active',b.dataset.filter===currentFilter));
  }
  function render(){renderStories();if(activeTab==='friends')renderFriends();if(activeTab==='chats')renderChats();if(activeTab==='snaps')renderSnaps();if(activeTab==='camera')renderCamera();}
  document.addEventListener('click',async e=>{
    const open=e.target.closest('[data-open-social]');if(open){showSocial(open.dataset.openSocial);return;}
    const tab=e.target.closest('[data-tab]');if(tab){if(tab.dataset.tab==='camera'||['friends','chats','snaps'].includes(tab.dataset.tab)){showSocial(tab.dataset.tab);return}}
    const add=e.target.closest('[data-add]');if(add){const f=demoFriends.find(x=>x.id===add.dataset.add);if(f){requests.push(f);render()}return}
    const accept=e.target.closest('[data-accept]');if(accept){friends.add(accept.dataset.accept);requests=requests.filter(x=>x.id!==accept.dataset.accept);render();return}
    const chat=e.target.closest('[data-chat], [data-select-chat]');if(chat){selectedChat=chat.dataset.chat||chat.dataset.selectChat;showSocial('chats');return}
    const filter=e.target.closest('[data-filter]');if(filter){currentFilter=filter.dataset.filter;applyFilter();return}
    const snap=e.target.closest('[data-open-snap]');if(snap){const name=demoFriends.find(x=>x.id===snap.dataset.openSnap)?.name||'Freund';const w=window.open('','_blank','width=420,height=640');if(w){w.document.write('<title>Demo-Snap</title><body style="margin:0;background:#091525;color:white;font:700 28px system-ui;display:grid;place-items:center;text-align:center">⚡<br>'+escapeHtml(name)+'s Demo-Snap<br><small style="font-size:14px">Nur eine Vorschau</small></body>');w.document.close()}else alert(name+'s Demo-Snap · Nur eine Vorschau');return}
    if(e.target.id==='snipzStartCamera'){if(!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia){alert('Die Kamera benötigt einen sicheren HTTPS-Aufruf und einen unterstützten Browser.');return}if(cameraStream){cameraStream.getTracks().forEach(t=>t.stop());cameraStream=null}try{const requestedStream=await navigator.mediaDevices.getUserMedia({video:{facingMode:'user'},audio:false});if(activeTab!=='camera'||!view.isConnected||!$('snipzCameraVideo')){requestedStream.getTracks().forEach(t=>t.stop());return}cameraStream=requestedStream;const v=$('snipzCameraVideo');v.srcObject=requestedStream;const placeholder=$('snipzCameraPlaceholder');if(placeholder)placeholder.style.display='none';await v.play().catch(()=>{});}catch(err){alert('Kamera konnte nicht gestartet werden. Prüfe die Browser-Berechtigung.');}return}
    if(e.target.id==='snipzSnapSend'){const f=$('snipzSnapFile').files[0];if(!f){alert('Wähle zuerst ein Foto oder Video aus.');return}alert('Dein Snap ist lokal vorbereitet. Das Versenden wird mit dem späteren Server aktiviert.');}
  });
  document.addEventListener('submit',e=>{
    if(e.target.id==='snipzChatForm'){e.preventDefault();const input=$('snipzMessage'),text=input.value.trim();if(!text)return;(messages[selectedChat] ||= []).push({mine:true,text});renderChats();}
  });
  // Keep the existing feed reachable through the home navigation.
  document.querySelectorAll('.side .nav').forEach(el=>{if(el.textContent.includes('Für dich'))el.addEventListener('click',showFeed)});
  const homeMobile=document.querySelector('.mobile button:first-child');if(homeMobile)homeMobile.addEventListener('click',showFeed);
  // Release camera tracks when leaving the camera tab.
  document.addEventListener('click',e=>{if(!e.target.closest('[data-open-social="camera"],[data-tab="camera"]')&&cameraStream&&activeTab==='camera'){cameraStream.getTracks().forEach(t=>t.stop());cameraStream=null;}});
  window.addEventListener('pagehide',()=>{if(cameraStream){cameraStream.getTracks().forEach(t=>t.stop());cameraStream=null;}});\n  document.addEventListener('visibilitychange',()=>{if(document.hidden&&cameraStream){cameraStream.getTracks().forEach(t=>t.stop());cameraStream=null;}});\n  showSocial('friends');showFeed();
})();