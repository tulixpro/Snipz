/* Snipz Social — private, local-only prototype. No social content is uploaded. */
(() => {
  const $ = id => document.getElementById(id);
  const main = document.querySelector('.layout main');
  if (!main || $('snipzSocialView')) return;

  const demoFriends = [
    {id:'mila',name:'Mila',handle:'@mila.blue',status:'Demo-Profil',letter:'M'},
    {id:'noah',name:'Noah',handle:'@noahx',status:'Demo-Profil',letter:'N'},
    {id:'lea',name:'Lea',handle:'@lea.vibes',status:'Demo-Profil',letter:'L'},
    {id:'emil',name:'Emil',handle:'@emil_07',status:'Demo-Profil',letter:'E'}
  ];
  let friends = new Set(['mila','noah','lea']);
  let requests = [{id:'emil',name:'Emil',handle:'@emil_07',letter:'E'}];
  let selectedChat = 'mila';
  let messages = {
    mila:[{mine:false,text:'Hey! Willkommen bei Snipz 👋'},{mine:true,text:'Danke! Was geht?'}],
    noah:[{mine:false,text:'Hast du das neue Video gesehen?'}],
    lea:[{mine:false,text:'Schick mir später einen Snap ✨'}]
  };
  let activeTab='friends', currentFilter='none', cameraStream=null;
  let ownStoryUrl=null, ownStoryCaption='', selectedPhotoUrl=null, selectedSticker='✨';
  const view=document.createElement('section');
  view.id='snipzSocialView';
  view.innerHTML = '<div class="social-head"><div><div class="social-muted">DEINE COMMUNITY</div><h2>Freunde & Nachrichten</h2></div><span class="social-muted">Snipz Social · private Demo</span></div>'+
    '<div class="social-card"><h3>Deine Stories</h3><div class="story-row" id="snipzStories"></div></div>'+
    '<div class="social-tabs" id="snipzTabs"><button class="social-tab active" data-tab="friends">👥 Freunde</button><button class="social-tab" data-tab="chats">💬 Chats</button><button class="social-tab" data-tab="snaps">⚡ Snaps</button><button class="social-tab" data-tab="camera">📸 Kamera & Studio</button></div>'+
    '<div id="snipzSocialContent"></div>';
  main.insertBefore(view, main.firstChild);

  const storyViewer=document.createElement('div');
  storyViewer.id='snipzStoryViewer';
  storyViewer.className='snipz-story-viewer';
  storyViewer.hidden=true;
  storyViewer.innerHTML='<div class="story-viewer-top"><div class="story-progress"><span id="snipzStoryProgress"></span></div><button type="button" id="snipzStoryClose" aria-label="Story schließen">×</button></div><div class="story-viewer-person" id="snipzStoryPerson"></div><div class="story-viewer-copy" id="snipzStoryCopy"></div><div class="story-viewer-bottom"><button type="button" id="snipzStoryPrev">‹</button><span>LOKALE DEMO-STORY</span><button type="button" id="snipzStoryNext">›</button></div>';
  view.appendChild(storyViewer);
  const content=$('snipzSocialContent');
  const nav=document.querySelector('.side');
  if(nav){const socialNav=document.createElement('div');socialNav.className='social-nav';socialNav.innerHTML='<div class="nav" data-open-social="friends">👥 <span class="label">Freunde</span></div><div class="nav" data-open-social="chats">💬 <span class="label">Chats</span></div><div class="nav" data-open-social="snaps">⚡ <span class="label">Snaps</span></div><div class="nav" data-open-social="camera">📸 <span class="label">Kamera</span></div>';nav.appendChild(socialNav);}
  const mobile=document.querySelector('.mobile');
  if(mobile){const b=document.createElement('button');b.type='button';b.textContent='💬';b.setAttribute('aria-label','Freunde und Chats');b.dataset.openSocial='chats';mobile.insertBefore(b,mobile.lastElementChild);}

  function stopCamera(){if(cameraStream){cameraStream.getTracks().forEach(t=>t.stop());cameraStream=null;}}
  function showSocial(tab){
    if(activeTab==='camera'&&tab!=='camera')stopCamera();
    activeTab=tab;view.classList.add('active');view.style.display='block';
    const feed=$('feed'),head=$('feedhead');if(feed)feed.style.display='none';if(head)head.style.display='none';
    document.querySelectorAll('[data-open-social]').forEach(x=>x.classList.toggle('active',x.dataset.openSocial===tab));
    document.querySelectorAll('[data-tab]').forEach(x=>x.classList.toggle('active',x.dataset.tab===tab));
    render();
  }
  function showFeed(){stopCamera();view.classList.remove('active');view.style.display='none';const feed=$('feed'),head=$('feedhead');if(feed)feed.style.display='';if(head)head.style.display='';document.querySelectorAll('[data-open-social]').forEach(x=>x.classList.remove('active'));}
  function escapeHtml(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
  function avatar(letter){return '<div class="social-avatar">'+letter+'</div>';}

  const demoStoryCards={
    mila:{title:'Kleiner Café-Stopp ☕',caption:'Heute mal eine Pause in der Sonne.',theme:'story-cafe'},
    noah:{title:'Abendlicher Himmel 🌇',caption:'Der Himmel sah gerade einfach toll aus.',theme:'story-sunset'},
    lea:{title:'Kreativzeit ✨',caption:'Ein bisschen Farbe für den Tag.',theme:'story-art'},
    emil:{title:'Musik an 🎧',caption:'Gute Laune für heute.',theme:'story-music'}
  };
  let storyOrder=[],storyIndex=0;
  function storyEntries(){
    const entries=demoFriends.filter(f=>friends.has(f.id)).map(f=>({id:f.id,person:f,story:demoStoryCards[f.id],own:false}));
    if(ownStoryUrl)entries.unshift({id:'own',person:{name:'Deine Story',handle:'Nur auf diesem Gerät',letter:'S'},story:{title:ownStoryCaption||'Dein Moment ✨',caption:'Private Vorschau auf diesem Gerät.',image:ownStoryUrl},own:true});
    return entries;
  }
  function openStory(id){storyOrder=storyEntries();storyIndex=Math.max(0,storyOrder.findIndex(x=>x.id===id));if(!storyOrder.length)return;storyViewer.hidden=false;storyViewer.setAttribute('aria-hidden','false');document.body.classList.add('story-open');renderStoryFrame();}
  function closeStory(){storyViewer.hidden=true;storyViewer.setAttribute('aria-hidden','true');document.body.classList.remove('story-open');}
  function renderStoryFrame(){
    const entry=storyOrder[storyIndex];if(!entry)return;
    $('snipzStoryPerson').innerHTML=avatar(entry.person.letter)+'<span><b>'+escapeHtml(entry.person.name)+'</b><small>'+escapeHtml(entry.person.handle)+' · '+(entry.own?'Privat':'Demo-Story')+'</small></span>';
    const scene=entry.story.image?'<div class="story-scene story-photo" style="background-image:url(&quot;'+entry.story.image+'&quot;)"><span>'+escapeHtml(entry.story.title)+'</span><p>'+escapeHtml(entry.story.caption)+'</p></div>':'<div class="story-scene '+entry.story.theme+'"><span>'+escapeHtml(entry.story.title)+'</span><p>'+escapeHtml(entry.story.caption)+'</p></div>';
    $('snipzStoryCopy').innerHTML=scene;
    $('snipzStoryProgress').style.width=((storyIndex+1)/storyOrder.length*100)+'%';
    $('snipzStoryPrev').disabled=storyIndex===0;$('snipzStoryNext').textContent=storyIndex===storyOrder.length-1?'✓':'›';
  }
  function renderStories(){
    const entries=storyEntries();
    $('snipzStories').innerHTML='<button class="story" data-tab="camera"><div class="story-ring your-story"><span>＋</span></div><small>Story erstellen</small></button>'+
      entries.map(x=>'<button class="story" data-story="'+x.id+'"><div class="story-ring"><span>'+(x.own?'<img src="'+x.story.image+'" alt="" class="story-thumb">':x.person.letter)+'</span></div><small>'+escapeHtml(x.person.name)+'</small></button>').join('');
  }
  function renderFriends(){
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Demo-Freunde · '+friends.size+'</h3>'+demoFriends.filter(f=>friends.has(f.id)).map(f=>'<div class="friend-row">'+avatar(f.letter)+'<div class="social-grow"><b>'+f.name+'</b><small>'+f.handle+' · '+f.status+'</small></div><button class="social-action" data-chat="'+f.id+'">Chat</button></div>').join('')+'</div><div class="social-card"><h3>Demo-Anfragen</h3>'+(requests.length?requests.map(f=>'<div class="friend-row">'+avatar(f.letter)+'<div class="social-grow"><b>'+f.name+'</b><small>'+f.handle+'</small></div><button class="social-action primary-action" data-accept="'+f.id+'">Annehmen</button></div>').join(''):'<div class="social-empty">Keine neuen Anfragen</div>')+'<h3 style="margin-top:18px">Demo-Profile entdecken</h3>'+demoFriends.filter(f=>!friends.has(f.id)&&!requests.some(r=>r.id===f.id)).map(f=>'<div class="friend-row">'+avatar(f.letter)+'<div class="social-grow"><b>'+f.name+'</b><small>'+f.handle+'</small></div><button class="social-action" data-add="'+f.id+'">Hinzufügen</button></div>').join('')+'</div></div><p class="social-note">Alle Profile und Anfragen sind erfundene Beispieldaten.</p>';
  }
  function renderChats(){
    const f=demoFriends.find(x=>x.id===selectedChat)||demoFriends[0];
    const list=demoFriends.filter(x=>friends.has(x.id)).map(x=>'<button class="chat-row" style="width:100%;text-align:left;background:none;color:inherit;border:0" data-select-chat="'+x.id+'">'+avatar(x.letter)+'<span class="social-grow"><b>'+x.name+'</b><small>'+(messages[x.id]?.at(-1)?.text||'Noch keine Nachrichten')+'</small></span><span class="social-muted">›</span></button>').join('');
    const bubbles=(messages[selectedChat]||[]).map(m=>'<div class="chat-bubble '+(m.mine?'mine':'')+'">'+escapeHtml(m.text)+'</div>').join('');
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Demo-Nachrichten</h3>'+list+'</div><div class="social-card"><h3>Chat mit '+f.name+'</h3><div class="chat-window" id="snipzChatWindow">'+(bubbles||'<div class="social-empty">Schreib die erste Demo-Nachricht!</div>')+'</div><form class="chat-compose" id="snipzChatForm"><input id="snipzMessage" maxlength="300" placeholder="Nachricht lokal testen …" autocomplete="off" required><button class="social-action primary-action">Testen</button></form></div></div><p class="social-note">Chats bleiben nur während dieser geöffneten Vorschau im Arbeitsspeicher. Es wird nichts verschickt.</p>';
    const win=$('snipzChatWindow');if(win)win.scrollTop=win.scrollHeight;
  }
  function renderSnaps(){
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Demo-Snaps</h3><div class="snap-row">'+avatar('M')+'<div class="social-grow"><b>Mila</b><small>Beispiel-Snap</small></div><button class="social-action primary-action" data-open-snap="mila">Ansehen</button></div><div class="snap-row">'+avatar('N')+'<div class="social-grow"><b>Noah</b><small>Beispiel-Snap</small></div><button class="social-action primary-action" data-open-snap="noah">Ansehen</button></div></div><div class="social-card"><h3>Eigenes Bild testen</h3><input class="social-input" style="width:100%" id="snipzSnapFile" type="file" accept="image/*"><div class="snap-preview" id="snipzSnapPreview">⚡<br>Wähle ein Bild von deinem Gerät</div><label class="social-note" for="snipzSnapCaption">Text auf deinem Bild</label><input class="social-input" id="snipzSnapCaption" maxlength="80" placeholder="Dein Text …" style="width:100%;margin:8px 0"><button class="social-action primary-action" id="snipzSnapPrepare" style="width:100%">Lokale Vorschau erstellen</button><p class="social-note">Das Bild wird nur in dieser Browser-Vorschau angezeigt und nicht hochgeladen.</p></div></div>';
  }
  function renderCamera(){
    content.innerHTML='<div class="social-grid"><div class="social-card"><h3>Kamera-Studio</h3><video id="snipzCameraVideo" class="camera-preview" autoplay muted playsinline hidden></video><div class="snap-preview" id="snipzCameraPlaceholder">📸<br>Dein Kamera-Studio</div><div class="filter-row"><button class="filter-chip active" data-filter="none">Original</button><button class="filter-chip" data-filter="cool">Cool</button><button class="filter-chip" data-filter="warm">Warm</button><button class="filter-chip" data-filter="mono">S/W</button><button class="filter-chip" data-filter="dream">Dream</button></div><div class="camera-controls"><button class="social-action primary-action" id="snipzStartCamera">Kamera starten</button><button class="social-action" id="snipzCapture" disabled>Foto aufnehmen</button><button class="social-action" id="snipzStopCamera" disabled>Kamera aus</button></div><p class="social-note">Die Kamera startet nur nach deinem Klick und deiner Browser-Erlaubnis. Es wird kein Ton aufgenommen.</p></div><div class="social-card"><h3>Dein Foto gestalten</h3><canvas id="snipzPhotoCanvas" class="photo-canvas" width="720" height="900"></canvas><div class="sticker-row"><button class="filter-chip" data-sticker="✨">✨</button><button class="filter-chip" data-sticker="💙">💙</button><button class="filter-chip" data-sticker="⭐">⭐</button><button class="filter-chip" data-sticker="🌈">🌈</button><button class="filter-chip" data-sticker="😎">😎</button></div><label class="social-note" for="snipzPhotoText">Dein Text</label><input class="social-input" id="snipzPhotoText" maxlength="70" placeholder="Schreib etwas Nettes …" style="width:100%;margin:8px 0"><div class="camera-controls"><button class="social-action" id="snipzAddStory" disabled>Als private Story zeigen</button><button class="social-action primary-action" id="snipzSavePhoto" disabled>Bild speichern</button></div><p class="social-note">Dein Foto bleibt hier lokal. „Bild speichern“ lädt es nur auf dein eigenes Gerät herunter.</p></div></div>';
    applyFilter();drawPhotoPlaceholder();
  }
  function drawPhotoPlaceholder(){const c=$('snipzPhotoCanvas');if(!c)return;const x=c.getContext('2d');x.fillStyle='#10213a';x.fillRect(0,0,c.width,c.height);x.fillStyle='#9fcfff';x.textAlign='center';x.font='bold 34px system-ui';x.fillText('Dein Snipz-Foto',c.width/2,c.height/2);}
  function applyFilter(){
    const v=$('snipzCameraVideo');if(v)v.className='camera-preview'+(currentFilter==='none'?'':' filter-'+currentFilter);
    document.querySelectorAll('[data-filter]').forEach(b=>b.classList.toggle('active',b.dataset.filter===currentFilter));
  }
  function render(){renderStories();if(activeTab==='friends')renderFriends();if(activeTab==='chats')renderChats();if(activeTab==='snaps')renderSnaps();if(activeTab==='camera')renderCamera();}
  function drawCapturedPhoto(){
    const v=$('snipzCameraVideo'),c=$('snipzPhotoCanvas');if(!v||!c||!v.videoWidth)return;
    const ctx=c.getContext('2d'),w=c.width,h=c.height,scale=Math.max(w/v.videoWidth,h/v.videoHeight),sw=w/scale,sh=h/scale,sx=(v.videoWidth-sw)/2,sy=(v.videoHeight-sh)/2;
    ctx.save();ctx.filter=({none:'none',cool:'saturate(1.3) hue-rotate(12deg) contrast(1.08)',warm:'sepia(.25) saturate(1.3)',mono:'grayscale(1) contrast(1.1)',dream:'hue-rotate(35deg) saturate(1.7) brightness(1.08)'})[currentFilter];ctx.drawImage(v,sx,sy,sw,sh,0,0,w,h);ctx.restore();
    drawOverlay(ctx,w,h);
    $('snipzSavePhoto').disabled=false;$('snipzAddStory').disabled=false;
  }
  function drawOverlay(ctx,w,h){
    const text=($('snipzPhotoText')?.value||'').trim();
    ctx.textAlign='center';ctx.shadowColor='#0009';ctx.shadowBlur=12;
    ctx.font='64px system-ui';ctx.fillText(selectedSticker,w/2,115);
    if(text){ctx.font='bold 42px system-ui';ctx.fillStyle='white';ctx.fillText(text.slice(0,35),w/2,h-95, w-60);}
    ctx.shadowBlur=0;
  }
  function savePhoto(){
    const c=$('snipzPhotoCanvas');if(!c)return;
    c.toBlob(blob=>{if(!blob)return;const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='snipz-foto.png';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);},'image/png');
  }

  document.addEventListener('click',async e=>{
    const open=e.target.closest('[data-open-social]');if(open){showSocial(open.dataset.openSocial);return;}
    if(e.target.closest('#snipzStoryClose')){closeStory();return;}
    if(e.target.closest('#snipzStoryPrev')){if(storyIndex>0){storyIndex--;renderStoryFrame()}return;}
    if(e.target.closest('#snipzStoryNext')){if(storyIndex<storyOrder.length-1){storyIndex++;renderStoryFrame()}else closeStory();return;}
    const story=e.target.closest('[data-story]');if(story){openStory(story.dataset.story);return;}
    const tab=e.target.closest('[data-tab]');if(tab&&['camera','friends','chats','snaps'].includes(tab.dataset.tab)){showSocial(tab.dataset.tab);return;}
    const add=e.target.closest('[data-add]');if(add){const f=demoFriends.find(x=>x.id===add.dataset.add);if(f){requests.push(f);render()}return;}
    const accept=e.target.closest('[data-accept]');if(accept){friends.add(accept.dataset.accept);requests=requests.filter(x=>x.id!==accept.dataset.accept);render();return;}
    const chat=e.target.closest('[data-chat],[data-select-chat]');if(chat){selectedChat=chat.dataset.chat||chat.dataset.selectChat;showSocial('chats');return;}
    const filter=e.target.closest('[data-filter]');if(filter){currentFilter=filter.dataset.filter;applyFilter();return;}
    const sticker=e.target.closest('[data-sticker]');if(sticker){selectedSticker=sticker.dataset.sticker;document.querySelectorAll('[data-sticker]').forEach(b=>b.classList.toggle('active',b===sticker));return;}
    if(e.target.id==='snipzStartCamera'){
      if(!navigator.mediaDevices?.getUserMedia){alert('Die Kamera braucht HTTPS und einen unterstützten Browser.');return;}
      try{stopCamera();cameraStream=await navigator.mediaDevices.getUserMedia({video:{facingMode:'user'},audio:false});if(activeTab!=='camera'){stopCamera();return;}const v=$('snipzCameraVideo');v.srcObject=cameraStream;v.hidden=false;$('snipzCameraPlaceholder').hidden=true;$('snipzCapture').disabled=false;$('snipzStopCamera').disabled=false;await v.play();}
      catch(err){stopCamera();alert('Kamera nicht verfügbar. Prüfe die Browser-Berechtigung oder frage eine erwachsene Person um Hilfe.');}return;
    }
    if(e.target.id==='snipzStopCamera'){stopCamera();const v=$('snipzCameraVideo');if(v){v.srcObject=null;v.hidden=true;}$('snipzCameraPlaceholder').hidden=false;$('snipzCapture').disabled=true;$('snipzStopCamera').disabled=true;return;}
    if(e.target.id==='snipzCapture'){drawCapturedPhoto();return;}
    if(e.target.id==='snipzSavePhoto'){savePhoto();return;}
    if(e.target.id==='snipzAddStory'){
      const c=$('snipzPhotoCanvas');if(!c)return;ownStoryUrl=c.toDataURL('image/jpeg',.88);ownStoryCaption=($('snipzPhotoText').value||'').trim();stopCamera();renderStories();alert('Deine private Demo-Story ist jetzt nur in dieser geöffneten Vorschau sichtbar.');return;
    }
    if(e.target.id==='snipzSnapPrepare'){
      const f=$('snipzSnapFile')?.files?.[0];if(!f||!f.type.startsWith('image/')){alert('Bitte wähle zuerst ein Bild aus.');return;}
      const reader=new FileReader();reader.onload=()=>{const preview=$('snipzSnapPreview');if(!preview)return;const caption=escapeHtml($('snipzSnapCaption').value||'');preview.innerHTML='<img src="'+reader.result+'" alt="Lokale Bildvorschau"><div class="snap-overlay">'+caption+'</div>';};reader.readAsDataURL(f);return;
    }
    const snap=e.target.closest('[data-open-snap]');if(snap){const name=demoFriends.find(x=>x.id===snap.dataset.openSnap)?.name||'Freund';alert(name+'s Beispiel-Snap · erfundene Vorschau.');return;}
  });
  document.addEventListener('input',e=>{if(e.target.id==='snipzPhotoText'&&$('snipzSavePhoto')&&!$('snipzSavePhoto').disabled){drawCapturedPhoto();}});
  document.addEventListener('submit',e=>{
    if(e.target.id==='snipzChatForm'){e.preventDefault();const input=$('snipzMessage'),text=input.value.trim();if(!text)return;(messages[selectedChat] ||= []).push({mine:true,text});renderChats();}
  });
  document.querySelectorAll('.side .nav').forEach(el=>{if(el.textContent.includes('Für dich'))el.addEventListener('click',showFeed)});
  const homeMobile=document.querySelector('.mobile button:first-child');if(homeMobile)homeMobile.addEventListener('click',showFeed);
  window.addEventListener('keydown',e=>{if(!storyViewer.hidden){if(e.key==='Escape')closeStory();if(e.key==='ArrowRight')$('snipzStoryNext').click();if(e.key==='ArrowLeft')$('snipzStoryPrev').click();}});
  window.addEventListener('pagehide',stopCamera);
  document.addEventListener('visibilitychange',()=>{if(document.hidden)stopCamera();});
  showSocial('friends');showFeed();
})();