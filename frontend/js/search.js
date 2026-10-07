/* Search: live results from /api/search (case-insensitive, partial match, done in C++). */
(function () {
  const state = { text: '', field: 'query', type: '', available: '' };

  Pages.search = {
    async render(view) {
      const fields = [['query', 'All fields'], ['title', 'Title'], ['author', 'Author'], ['isbn', 'ISBN / ISSN'], ['genre', 'Genre']];
      view.innerHTML =
        '<div class="search-hero"><div class="search-box">' + icon('search', 22) + '<input class="input" id="sq" autocomplete="off" placeholder="Search books, authors, ISBN, journals…" value="' + esc(state.text) + '"></div>' +
        '<div style="display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-top:14px"><div class="chips" id="sFields"></div>' +
        '<select class="input" id="sType"><option value="">All types</option><option value="BOOK">Books</option><option value="EBOOK">E-books</option><option value="JOURNAL">Journals</option></select>' +
        '<select class="input" id="sAvail"><option value="">Any availability</option><option value="true">Available now</option><option value="false">Unavailable</option></select></div></div>' +
        '<div id="sResults"></div>';
      view.querySelector('#sType').value = state.type;
      view.querySelector('#sAvail').value = state.available;
      const drawChips = () => { view.querySelector('#sFields').innerHTML = fields.map((f) => '<button class="chip ' + (state.field === f[0] ? 'on' : '') + '" data-action="field" data-v="' + f[0] + '">' + f[1] + '</button>').join(''); };
      drawChips();

      let token = 0, results = [];
      const run = async () => {
        const mine = ++token, box = view.querySelector('#sResults');
        const text = state.text.trim();
        if (!text) { box.innerHTML = '<div class="card" style="margin-top:18px">' + emptyHtml('Start typing to search', 'Matches are case-insensitive and partial: “harry” finds “Harry Potter”.', 'search') + '</div>'; return; }
        box.innerHTML = '<div style="margin-top:18px">' + loadingHtml('Searching…') + '</div>';
        try {
          const params = { type: state.type, available: state.available };
          params[state.field] = text;
          const res = (await API.search(params)).data;
          if (mine !== token || !box.isConnected) return;
          results = res.results;
          box.innerHTML = results.length
            ? '<div class="hint" style="margin-top:14px">' + res.count + ' result(s) for “' + esc(text) + '”</div><div class="result-grid" style="margin-top:8px">' + results.map((r) =>
              '<div class="card result-card"><div class="result-meta">' + typeBadge(r.type) + statusBadge(r.availability) + '<span class="cell-sub" style="margin-left:auto">' + r.availableCopies + '/' + r.totalCopies + ' copies</span></div>' +
              '<h3>' + highlight(r.title, text) + '</h3><div class="cell-sub">by ' + highlight(r.author, text) + '</div>' +
              '<div class="result-meta"><span class="mono">' + r.identifierLabel + ' ' + highlight(r.isbn, text) + '</span><span class="cell-sub">· ' + highlight(r.genre, text) + ' · ' + r.publicationYear + '</span></div>' +
              '<div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:auto"><a class="btn btn-sm btn-ghost" href="#/resources/' + r.id + '">' + icon('eye', 15) + ' View</a>' +
              (r.available ? '<button class="btn btn-sm btn-primary" data-action="borrow" data-id="' + r.id + '">' + icon('repeat', 15) + ' Borrow</button>' : '<button class="btn btn-sm btn-cyan" data-action="reserve" data-id="' + r.id + '">' + icon('bookmark', 15) + ' Reserve</button>') + '</div></div>').join('') + '</div>'
            : '<div class="card" style="margin-top:18px">' + emptyHtml('No results', 'Nothing matched “' + text + '”. Try another field or a shorter term.', 'search') + '</div>';
        } catch (e) { if (mine === token) box.innerHTML = '<div class="card" style="margin-top:18px">' + errorHtml(e.message) + '</div>'; }
      };
      run();

      const live = debounce(run, 250);
      view.oninput = (e) => { if (e.target.id === 'sq') { state.text = e.target.value; live(); } };
      view.onchange = (e) => { if (e.target.id === 'sType') { state.type = e.target.value; run(); } if (e.target.id === 'sAvail') { state.available = e.target.value; run(); } };
      const byId = (el) => results.find((r) => r.id === parseInt(el.dataset.id, 10));
      App.delegate(view, {
        retry: run,
        field: (el) => { state.field = el.dataset.v; drawChips(); run(); },
        borrow: (el) => Flows.borrow(byId(el)),
        reserve: (el) => Flows.reserve(byId(el)),
      });
      const input = view.querySelector('#sq'); if (input) input.focus();
    },
  };
})();
