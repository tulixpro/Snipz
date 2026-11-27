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
