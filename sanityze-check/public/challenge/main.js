const STORAGE_KEY = "sanityze-check-notes";
const MAX_NOTES = 50;
const MAX_NOTE_LENGTH = 1000;
const DEFAULT_LIMIT = 10;
const DEFAULT_SORT = "newest";

function getInsertedAt(note, fallback) {
    const insertedAt = note.createdAt;
    if (typeof insertedAt === "number") return insertedAt;

    const parsed = Date.parse(insertedAt);
    return Number.isFinite(parsed) ? parsed : fallback;
}

function sortNotes(notes) {
    return notes
        .map((note, index) => ({ note, index }))
        .sort((a, b) =>
            getInsertedAt(a.note, a.index) - getInsertedAt(b.note, b.index) ||
            a.index - b.index,
        )
        .map(({ note }) => note);
}

function loadNotes() {
    try {
        const notes = JSON.parse(localStorage.getItem(STORAGE_KEY)) || [];
        return Array.isArray(notes) ? sortNotes(notes).slice(0, MAX_NOTES) : [];
    } catch {
        return [];
    }
}

function saveNotes(notes) {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(sortNotes(notes).slice(0, MAX_NOTES)));
}

function getViewOptions(url = new URL(window.location.href)) {
    const requestedLimit = Number(url.searchParams.get("limit"));
    const limit = Number.isInteger(requestedLimit) && requestedLimit > 0
        ? Math.min(requestedLimit, MAX_NOTES)
        : DEFAULT_LIMIT;
    const sort = url.searchParams.get("sort") === "oldest" ? "oldest" : DEFAULT_SORT;

    return { limit, sort };
}

function selectNotes(notes, { limit, sort } = getViewOptions()) {
    const selected = sortNotes(notes);
    if (sort === "newest") selected.reverse();
    return selected.slice(0, limit);
}

function createNoteId() {
    return Math.random().toString(36).slice(2);
}

function addNote(content) {
    const notes = loadNotes();
    if (notes.length >= MAX_NOTES) return false;

    notes.push({
        id: createNoteId(),
        content: (content || "").slice(0, MAX_NOTE_LENGTH),
        createdAt: Date.now(),
    });
    saveNotes(notes);
    return true;
}

function deleteNote(id) {
    saveNotes(loadNotes().filter((note) => note.id !== id));
}

function renderNotes(notes) {
    const notesContainer = document.getElementById("notes");
    if (!notesContainer) return;

    notesContainer.querySelectorAll(".stored-note").forEach((note) => note.remove());

    if (notes.length === 0) {
        const emptyMessage = document.createElement("p");
        emptyMessage.className = "stored-note";
        emptyMessage.textContent = "No notes found.";
        notesContainer.appendChild(emptyMessage);
        return;
    }

    for (const note of notes) {
        const noteElement = document.createElement("div");
        noteElement.className = "note stored-note";

        const content = note.content || "";

        noteElement.setHTML(`<p>${content}</p>`);

        const deleteButton = document.createElement("button");
        deleteButton.type = "button";
        deleteButton.textContent = "Delete";
        deleteButton.addEventListener("click", () => {
            deleteNote(note.id);
            noteElement.remove();
        });

        noteElement.appendChild(deleteButton);
        notesContainer.appendChild(noteElement);
    }
}

function handleAddRoute() {
    const url = new URL(window.location.href);
    const action = url.searchParams.get("action") || "";
    if (action !== "add") return false;

    const content = url.searchParams.get("content") || "";
    addNote(content);

    window.location.replace("/");
    return true;
}

function filterNotes(query) {
    return loadNotes().filter((note) =>
        (note.content || "").startsWith(query),
    );
}

function handleSearchRoute() {
    const url = new URL(window.location.href);
    const action = url.searchParams.get("action") || "";
    if (action !== "search") return false;

    const query = url.searchParams.get("query") || "";
    const options = getViewOptions(url);
    const searchInput = document.getElementById("search-input");
    const limitInput = document.getElementById("limit-input");
    const sortInput = document.getElementById("sort-input");

    if (searchInput) searchInput.value = query;
    if (limitInput) limitInput.value = options.limit;
    if (sortInput) sortInput.value = options.sort;

    renderNotes(selectNotes(filterNotes(query), options));
    return true;
}

function setupAddNoteForm() {
    const form = document.getElementById("add-note-form");
    const contentInput = document.getElementById("content-input");
    if (!form || !contentInput) return;

    form.addEventListener("submit", (event) => {
        event.preventDefault();
        if (addNote(contentInput.value || "")) {
            form.reset();
        }
        renderNotes(selectNotes(loadNotes()));
    });
}

document.addEventListener("DOMContentLoaded", () => {
    if (handleAddRoute()) return;

    setupAddNoteForm();

    if (handleSearchRoute()) return;

    renderNotes(selectNotes(loadNotes()));
});
