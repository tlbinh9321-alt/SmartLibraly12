/* Resources: list with filters, add/edit modal, detail page. */
(function () {
  const filters = { q: '', type: '', availability: '', genre: '' };
  let genresCache = [];

  const extraFields = {
    BOOK: [['publisher', 'Publisher', 'text'], ['pageCount', 'Page count', 'number'], ['edition', 'Edition', 'text']],
    EBOOK: [['fileFormat', 'File format', ['PDF', 'EPUB', 'MOBI']], ['fileSizeMB', 'File size (MB)', 'number'], ['fileReference', 'Download URL / file reference', 'text', true], ['licenseType', 'License type', ['Perpetual', 'Subscription', 'Limited']]],
    JOURNAL: [['issn', 'ISSN (e.g. 0028-0836)', 'text'], ['volume', 'Volume', 'number'], ['issue', 'Issue', 'number'], ['academicField', 'Academic field', 'text', true]],
  };

  function fieldHtml(def, value) {
    const [key, label, kind, full] = def;
    const v = value === undefined || value === null ? '' : value;
    const control = Array.isArray(kind)
      ? '<select class="input" name="' + key + '">' + kind.map((o) => '<option ' + (String(v).toUpperCase() === o.toUpperCase() ? 'selected' : '') + '>' + o + '</option>').join('') + '</select>'
      : '<input class="input" name="' + key + '" type="' + kind + '" ' + (kind === 'number' ? 'min="0" step="any"' : '') + ' value="' + esc(v) + '">';
    return '<div class="field ' + (full ? 'full' : '') + '"><label>' + label + '</label>' + control + '</div>';
  }

  function formBody(r, type) {
    const v = r || { extra: {} };
    return '<div data-error></div><div class="form-grid">' +
      '<div class="field"><label>Resource type</label><select class="input" name="type" ' + (r ? 'disabled' : '') + '>' + ['BOOK', 'EBOOK', 'JOURNAL'].map((t) => '<option ' + (t === type ? 'selected' : '') + '>' + t + '</option>').join('') + '</select></div>' +
      '<div class="field"><label>Total copies</label><input class="input" name="totalCopies" type="number" min="0" value="' + (r ? r.totalCopies : 1) + '"></div>' +
      '<div class="field full"><label>Title</label><input class="input" name="title" value="' + esc(v.title) + '" required></div>' +
      '<div class="field"><label>Author</label><input class="input" name="author" value="' + esc(v.author) + '" required></div>' +
      '<div class="field"><label>Genre</label><input class="input" name="genre" list="genreList" value="' + esc(v.genre) + '" required><datalist id="genreList">' + genresCache.map((g) => '<option value="' + esc(g) + '">').join('') + '</datalist></div>' +
      '<div class="field"><label>Publication year</label><input class="input" name="publicationYear" type="number" value="' + esc(v.publicationYear || new Date().getFullYear()) + '"></div>' +
      (type === 'JOURNAL' ? '' : '<div class="field"><label>ISBN</label><input class="input" name="isbn" value="' + esc(r ? r.isbn : '') + '" placeholder="978-0-13-235088-4"></div>') +
      extraFields[type].map((d) => fieldHtml(d, v.extra ? v.extra[d[0]] : '')).join('') + '</div>';
  }

  function collect(form, type) {
    const out = { type: type };
    new FormData(form).forEach((val, key) => { out[key] = val; });
    out.type = type;
    return out;
  }

  async function openForm(resource) {
    genresCache = (await API.genres()).data;
    let type = resource ? resource.type : 'BOOK';
    let current = resource;
    const modal = Modal.open({
      title: resource ? 'Edit resource #' + resource.id : 'Add resource', size: 'wide',
      body: '<form id="resForm">' + formBody(resource, type) + '</form>',
      footer: '<button class="btn btn-ghost" data-close>Cancel</button><button class="btn btn-primary" data-action="save">' + (resource ? 'Save changes' : 'Create resource') + '</button>',
      onChange: (e, m) => {
        if (e.target.name === 'type' && !resource) {
          const snapshot = collect(m.$('#resForm'), type);
          type = e.target.value;
          current = { title: snapshot.title, author: snapshot.author, genre: snapshot.genre, publicationYear: snapshot.publicationYear, totalCopies: snapshot.totalCopies, extra: {} };
          m.$('#resForm').innerHTML = formBody(null, type);
          ['title', 'author', 'genre', 'publicationYear', 'totalCopies'].forEach((k) => { const el = m.$('[name=' + k + ']'); if (el && snapshot[k] !== undefined) el.value = snapshot[k]; });
        }
      },
      onSubmit: (f, m) => m.$('[data-action=save]').click(),
      handlers: {
        save: async (btn, e, m) => {
          btn.disabled = true; m.error('');
          try {
            const body = collect(m.$('#resForm'), type);
            const r = resource ? await API.updateResource(resource.id, body) : await API.createResource(body);
            Toast.show(r.message, 'success');
            m.close();
            App.refresh();
          } catch (err) { m.error(err.message); btn.disabled = false; }
        },
      },
    });
    return modal;
  }

  async function confirmDelete(r) {
    const ok = await confirmDialog({ title: 'Delete resource', message: 'Delete “' + r.title + '” permanently? Resources with active loans or open reservations cannot be deleted.', confirmText: 'Delete', danger: true });
    if (!ok) return false;
    return App.run(async () => { const res = await API.deleteResource(r.id); Toast.show(res.message, 'success'); return true; });
  }

  function tableRows(list) {
    return list.map((r) => {
      const canBorrow = r.available, canReserve = !r.available;
      return '<tr>' +
        '<td data-label="ID" class="num">' + r.id + '</td>' +
        '<td data-label="Title" class="cell-main"><button class="link-btn" data-action="view" data-id="' + r.id + '">' + esc(r.title) + '</button></td>' +
        '<td data-label="Author">' + esc(r.author) + '</td>' +
        '<td data-label="Type">' + typeBadge(r.type) + '</td>' +
        '<td data-label="' + r.identifierLabel + '" class="mono">' + esc(r.isbn) + '</td>' +
        '<td data-label="Availability">' + statusBadge(r.availability) + '</td>' +
        '<td data-label="Copies" class="num">' + r.availableCopies + ' / ' + r.totalCopies + '</td>' +
        '<td class="actions">' +
        '<button class="icon-btn" title="View" aria-label="View" data-action="view" data-id="' + r.id + '">' + icon('eye') + '</button>' +
        '<button class="icon-btn" title="Edit" aria-label="Edit" data-action="edit" data-id="' + r.id + '">' + icon('edit') + '</button>' +
        '<button class="icon-btn" title="' + (canBorrow ? 'Borrow' : 'No copy available') + '" aria-label="Borrow" data-action="borrow" data-id="' + r.id + '" ' + (canBorrow ? '' : 'disabled') + '>' + icon('repeat') + '</button>' +
        '<button class="icon-btn" title="' + (canReserve ? 'Reserve' : 'Available now — borrow instead') + '" aria-label="Reserve" data-action="reserve" data-id="' + r.id + '" ' + (canReserve ? '' : 'disabled') + '>' + icon('bookmark') + '</button>' +
        '<button class="icon-btn danger" title="Delete" aria-label="Delete" data-action="delete" data-id="' + r.id + '">' + icon('trash') + '</button></td></tr>';
    }).join('');
  }

  Pages.resources = {
    async render(view, id) {
      if (id) return Pages.resources.renderDetail(view, parseInt(id, 10));
      const alive = App.guard();
      genresCache = (await API.genres()).data;
      if (!alive()) return;
      let list = [];
      const typeOpts = ['', 'BOOK', 'EBOOK', 'JOURNAL'].map((t) => '<option value="' + t + '" ' + (filters.type === t ? 'selected' : '') + '>' + (t || 'All types') + '</option>').join('');
      const genreOpts = ['<option value="">All genres</option>'].concat(genresCache.map((g) => '<option ' + (filters.genre === g ? 'selected' : '') + '>' + esc(g) + '</option>')).join('');
      view.innerHTML =
        '<div class="page-head"><div><h2>Library Resources</h2><p>Books, e-books and academic journals.</p></div><div class="spacer"><button class="btn btn-primary" data-action="add">' + icon('plus', 16) + ' Add Resource</button></div></div>' +
        '<div class="card"><div class="filters"><div class="search-box">' + icon('search', 18) + '<input class="input" id="fq" placeholder="Search title, author, ISBN, genre…" value="' + esc(filters.q) + '"></div>' +
        '<select class="input" id="ftype">' + typeOpts + '</select>' +
        '<select class="input" id="favail"><option value="">Any availability</option><option value="available" ' + (filters.availability === 'available' ? 'selected' : '') + '>Available</option><option value="unavailable" ' + (filters.availability === 'unavailable' ? 'selected' : '') + '>Unavailable</option></select>' +
        '<select class="input" id="fgenre">' + genreOpts + '</select></div>' +
        '<div id="resTable">' + loadingHtml() + '</div></div>';

      let token = 0;
      const load = async () => {
        const mine = ++token;
        const box = view.querySelector('#resTable');
        try {
          const res = await API.resources(filters);
          if (mine !== token || !box.isConnected) return;
          list = res.data;
          box.innerHTML = list.length
            ? '<div class="table-wrap"><table class="rt"><thead><tr><th>ID</th><th>Title</th><th>Author</th><th>Type</th><th>ISBN / ISSN</th><th>Availability</th><th>Copies</th><th style="text-align:right">Actions</th></tr></thead><tbody>' + tableRows(list) + '</tbody></table></div><div class="hint" style="padding:10px 22px 16px">' + list.length + ' resource(s)</div>'
            : emptyHtml('No resources match', 'Try different filters, or add a new resource.', 'book');
        } catch (e) { if (mine === token) box.innerHTML = errorHtml(e.message); }
      };
      load();

      const byId = (el) => list.find((r) => r.id === parseInt(el.dataset.id, 10));
      view.oninput = debounce((e) => { if (e.target.id === 'fq') { filters.q = e.target.value; load(); } }, 250);
      view.onchange = (e) => {
        const map = { ftype: 'type', favail: 'availability', fgenre: 'genre' };
        if (map[e.target.id]) { filters[map[e.target.id]] = e.target.value; load(); }
      };
      App.delegate(view, {
        retry: load,
        add: () => App.run(() => openForm(null)),
        view: (el) => { location.hash = '#/resources/' + el.dataset.id; },
        edit: (el) => App.run(() => openForm(byId(el))),
        borrow: (el) => Flows.borrow(byId(el)),
        reserve: (el) => Flows.reserve(byId(el)),
        'delete': async (el) => { if (await confirmDelete(byId(el))) load(); },
      });
    },

    async renderDetail(view, id) {
      const alive = App.guard();
      const r = (await API.resource(id)).data;
      if (!alive()) return;
      $('#pageTitle').textContent = r.title;
      const fact = (k, v) => '<div class="fact"><div class="k">' + k + '</div><div class="v">' + v + '</div></div>';
      const extras = Object.keys(r.extra).map((k) => fact(k.replace(/([A-Z])/g, ' $1').replace(/^./, (c) => c.toUpperCase()), esc(r.extra[k] || '—'))).join('');
      const history = r.borrowingHistory.length
        ? '<div class="table-wrap"><table class="rt"><thead><tr><th>Loan</th><th>Member</th><th>Borrowed</th><th>Due</th><th>Returned</th><th>Status</th></tr></thead><tbody>' + r.borrowingHistory.map((l) => '<tr><td data-label="Loan">#' + l.id + '</td><td data-label="Member"><a href="#/members/' + l.memberId + '">' + esc(l.memberName) + '</a></td><td data-label="Borrowed">' + fmtDate(l.borrowDate) + '</td><td data-label="Due">' + fmtDate(l.dueDate) + '</td><td data-label="Returned">' + fmtDate(l.returnDate) + '</td><td data-label="Status">' + statusBadge(l.status) + '</td></tr>').join('') + '</tbody></table></div>'
        : emptyHtml('Never borrowed', 'No loan history for this resource yet.', 'repeat');
      const queueEntry = { queue: r.reservationQueue, ready: r.readyReservations };
      view.innerHTML =
        '<a class="back-link" href="#/resources">' + icon('back', 16) + ' All resources</a>' +
        '<div class="card" style="margin-bottom:18px"><div class="card-pad" style="display:flex;gap:14px;flex-wrap:wrap;align-items:flex-start"><div style="min-width:0;flex:1 1 320px"><div style="display:flex;gap:8px;flex-wrap:wrap;margin-bottom:8px">' + typeBadge(r.type) + statusBadge(r.availability) + '</div><h2 style="font-size:26px">' + esc(r.title) + '</h2><p style="color:var(--muted);margin:4px 0 0">by ' + esc(r.author) + '</p></div>' +
        '<div style="display:flex;gap:10px;flex-wrap:wrap"><button class="btn btn-primary" data-action="borrow" ' + (r.available ? '' : 'disabled') + '>' + icon('repeat', 16) + ' Borrow</button><button class="btn btn-cyan" data-action="reserve" ' + (r.available ? 'disabled' : '') + '>' + icon('bookmark', 16) + ' Reserve</button><button class="btn btn-ghost" data-action="edit">' + icon('edit', 16) + ' Edit</button></div></div>' +
        '<div class="facts">' + fact(r.identifierLabel, '<span class="mono">' + esc(r.isbn) + '</span>') + fact('Genre', esc(r.genre)) + fact('Publication year', r.publicationYear) + fact('Total copies', r.totalCopies) + fact('Available copies', r.availableCopies + (r.heldCopies ? ' (' + r.heldCopies + ' held for a reservation)' : '')) + fact('Late fee', money(r.lateFeeRate) + ' / day') + extras + '</div>' +
        '<div class="card-pad" style="padding-top:0"><div class="hint" style="font-size:13.5px">' + esc(r.description) + '</div></div></div>' +
        '<div class="grid two"><div class="card"><div class="card-head"><h2>Borrowing history</h2></div>' + history + '</div>' +
        '<div class="card"><div class="card-head"><h2>Reservation queue</h2></div><div style="padding:14px 22px 22px">' + queueHtml(queueEntry, false) + '</div></div></div>';
      App.delegate(view, {
        borrow: () => Flows.borrow(r), reserve: () => Flows.reserve(r),
        edit: () => App.run(() => openForm(r)),
      });
    },
  };
})();
