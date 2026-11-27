const searchInput = document.querySelector("#search");
const filters = [...document.querySelectorAll(".filter")];
const cards = [...document.querySelectorAll(".game-card")];
const emptyState = document.querySelector("#empty-state");
const resultCount = document.querySelector("#result-count");
let activeFilter = "all";

function updateLibrary() {
  const term = searchInput.value.trim().toLowerCase();
  let visible = 0;
  for (const card of cards) {
    const matchesText = card.dataset.name.includes(term);
    const matchesCategory = activeFilter === "all" || card.dataset.category === activeFilter;
    const show = matchesText && matchesCategory;
    card.hidden = !show;
    if (show) visible += 1;
  }
  emptyState.hidden = visible !== 0;
  resultCount.textContent = String(visible).padStart(2, "0") + (visible === 1 ? " SPIEL" : " SPIELE");
}
searchInput.addEventListener("input", updateLibrary);
filters.forEach((button) => button.addEventListener("click", () => {
  activeFilter = button.dataset.filter;
  filters.forEach((item) => item.classList.toggle("active", item === button));
  updateLibrary();
}));
updateLibrary();

// Gamepad API: use a connected controller to move focus and activate links/buttons.
const controllerStatus = document.querySelector("#controller-status");
let previousButtons = [];
let lastMove = 0;
let lastAction = 0;
let gamepadFrame = 0;

function visibleFocusable() {
  return [...document.querySelectorAll('a[href], button:not(:disabled), input:not(:disabled)')]
    .filter((element) => element.getClientRects().length && !element.closest('[hidden]'));
}
function moveFocus(direction) {
  const items = visibleFocusable();
  if (!items.length) return;
  const current = items.indexOf(document.activeElement);
  const next = current < 0 ? 0 : (current + direction + items.length) % items.length;
  items[next].focus({ preventScroll: false });
}
function gamepadLoop(timestamp) {
  const pads = navigator.getGamepads ? [...navigator.getGamepads()].filter(Boolean) : [];
  const pad = pads[0];
  if (pad) {
    controllerStatus.classList.add("connected");
    controllerStatus.innerHTML = '<span class="status-dot"></span> Controller verbunden: ' + (pad.id || "Gamepad");
    const pressed = (index) => Boolean(pad.buttons[index] && pad.buttons[index].pressed);
    const left = pressed(14) || pad.axes[0] < -0.65;
    const right = pressed(15) || pad.axes[0] > 0.65;
    const up = pressed(12) || pad.axes[1] < -0.65;
    const down = pressed(13) || pad.axes[1] > 0.65;
    const direction = left || up ? -1 : right || down ? 1 : 0;
    if (direction && timestamp - lastMove > 230) {
      moveFocus(direction);
      lastMove = timestamp;
    }
    const aPressed = pressed(0) && !previousButtons[0];
    const bPressed = pressed(1) && !previousButtons[1];
    if (aPressed && timestamp - lastAction > 180) {
      const active = document.activeElement;
      if (active && (active.matches("a[href],button"))) active.click();
      lastAction = timestamp;
    }
    if (bPressed && timestamp - lastAction > 180) {
      if (document.activeElement && document.activeElement.matches("input")) document.activeElement.blur();
      else window.scrollTo({ top: 0, behavior: "smooth" });
      lastAction = timestamp;
    }
    previousButtons = pad.buttons.map((button) => button.pressed);
  } else {
    controllerStatus.classList.remove("connected");
    controllerStatus.textContent = "Noch kein Controller erkannt";
    previousButtons = [];
  }
  gamepadFrame = requestAnimationFrame(gamepadLoop);
}
window.addEventListener("gamepadconnected", (event) => {
  controllerStatus.classList.add("connected");
  controllerStatus.textContent = "Controller verbunden: " + event.gamepad.id;
});
window.addEventListener("gamepaddisconnected", () => {
  controllerStatus.classList.remove("connected");
  controllerStatus.textContent = "Noch kein Controller erkannt";
});
if ("getGamepads" in navigator) gamepadFrame = requestAnimationFrame(gamepadLoop);
