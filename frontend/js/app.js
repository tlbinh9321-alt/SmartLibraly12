/* App shell: icons, helpers, router, toasts, modals, charts and shared user flows. */
(function (root) {
  'use strict';

  // ---- icons (stroke icons, 24x24) -------------------------------------------------------
  const ICONS = {
    grid: '<rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/>',
    book: '<path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/>',
    users: '<path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/>',
    user: '<path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/>',
    repeat: '<polyline points="17 1 21 5 17 9"/><path d="M3 11V9a4 4 0 0 1 4-4h14"/><polyline points="7 23 3 19 7 15"/><path d="M21 13v2a4 4 0 0 1-4 4H3"/>',
    bookmark: '<path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/>',
    search: '<circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>',
    chart: '<line x1="18" y1="20" x2="18" y2="10"/><line x1="12" y1="20" x2="12" y2="4"/><line x1="6" y1="20" x2="6" y2="14"/>',
    settings: '<line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/><line x1="1" y1="14" x2="7" y2="14"/><line x1="9" y1="8" x2="15" y2="8"/><line x1="17" y1="16" x2="23" y2="16"/>',
    plus: '<line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/>',
    eye: '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/>',
    edit: '<path d="M12 20h9"/><path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z"/>',
    trash: '<polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/><path d="M10 11v6"/><path d="M14 11v6"/><path d="M9 6V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v2"/>',
    clock: '<circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/>',
    alert: '<path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/>',
    dollar: '<line x1="12" y1="1" x2="12" y2="23"/><path d="M17 5H9.5a3.5 3.5 0 0 0 0 7h5a3.5 3.5 0 0 1 0 7H6"/>',
    check: '<polyline points="20 6 9 17 4 12"/>',
    x: '<line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/>',
    menu: '<line x1="3" y1="12" x2="21" y2="12"/><line x1="3" y1="6" x2="21" y2="6"/><line x1="3" y1="18" x2="21" y2="18"/>',
    download: '<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/>',
    upload: '<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/>',
    save: '<path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/>',
    back: '<line x1="19" y1="12" x2="5" y2="12"/><polyline points="12 19 5 12 12 5"/>',
    refresh: '<polyline points="23 4 23 10 17 10"/><path d="M20.49 15a9 9 0 1 1-2.12-9.36L23 10"/>',
    layers: '<polygon points="12 2 2 7 12 12 22 7 12 2"/><polyline points="2 17 12 22 22 17"/><polyline points="2 12 12 17 22 12"/>',
    inbox: '<polyline points="22 12 16 12 14 15 10 15 8 12 2 12"/><path d="M5.45 5.11L2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z"/>',
    file: '<path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/>',
  };
  const icon = (name, size) => '<svg class="ic" width="' + (size || 18) + '" height="' + (size || 18) + '" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' + (ICONS[name] || '') + '</svg>';

  // ---- small helpers ---------------------------------------------------------------------
  const esc = (s) => String(s === undefined || s === null ? '' : s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const money = (n) => '$' + Number(n || 0).toFixed(2);
  const fmtDate = (iso) => { if (!iso) return '—'; const p = String(iso).split('-'); return p.length === 3 ? p[2] + '/' + p[1] + '/' + p[0] : iso; };
  const $ = (sel, scope) => (scope || document).querySelector(sel);
  const typeBadge = (t) => '<span class="badge type-' + esc(String(t).toLowerCase()) + '">' + esc(t) + '</span>';
  const statusBadge = (s) => '<span class="badge st-' + esc(String(s).toLowerCase()) + '">' + esc(s) + '</span>';
  const debounce = (fn, ms) => { let t; return function () { const a = arguments, self = this; clearTimeout(t); t = setTimeout(function () { fn.apply(self, a); }, ms); }; };
  const highlight = (text, term) => {
    const t = (term || '').trim();
    if (!t) return esc(text);
    const re = new RegExp('(' + t.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + ')', 'gi');
    return String(text === undefined || text === null ? '' : text).split(re).map((p, i) => (i % 2 ? '<mark>' + esc(p) + '</mark>' : esc(p))).join('');
  };
  const loadingHtml = (msg) => '<div class="state"><div class="spinner"></div><div>' + esc(msg || 'Loading…') + '</div></div>';
  const emptyHtml = (title, hint, ic) => '<div class="state"><div class="state-ic">' + icon(ic || 'inbox', 26) + '</div><h3>' + esc(title) + '</h3><div>' + esc(hint || '') + '</div></div>';
  const errorHtml = (msg) => '<div class="state"><div class="state-ic tone-red">' + icon('alert', 26) + '</div><h3>Something went wrong</h3><div>' + esc(msg) + '</div><p><button class="btn btn-primary btn-sm" data-action="retry">' + icon('refresh', 16) + ' Try again</button></p></div>';

  // ---- toasts ------------------------------------------------------------------------------
  const Toast = {
    show(message, type) {
      const host = $('#toasts');
      if (!host) return;
      const el = document.createElement('div');
      el.className = 'toast ' + (type || 'info');
      el.innerHTML = '<span>' + icon(type === 'error' ? 'alert' : type === 'success' ? 'check' : 'bookmark', 18) + '</span><span class="t-msg">' + esc(message) + '</span>';
      const remove = () => { el.classList.add('leaving'); setTimeout(() => el.remove(), 260); };
      el.addEventListener('click', remove);
      host.appendChild(el);
      setTimeout(remove, type === 'error' ? 7000 : 4500);
    },
  };

  // ---- modal -------------------------------------------------------------------------------
  const openModals = [];
  const Modal = {
    open(opts) {
      const wrap = document.createElement('div');
      wrap.className = 'modal-backdrop';
      const tone = opts.tone ? '<span class="modal-ic tone-' + opts.tone + '">' + icon(opts.icon || 'bookmark', 22) + '</span>' : '';
      wrap.innerHTML = '<div class="modal ' + (opts.size || '') + '" role="dialog" aria-modal="true"><div class="modal-head">' + tone +
        '<h3>' + esc(opts.title) + '</h3><button class="icon-btn" data-close aria-label="Close">' + icon('x') + '</button></div>' +
        '<div class="modal-body">' + (opts.body || '') + '</div><div class="modal-foot">' + (opts.footer || '') + '</div></div>';
      $('#modalRoot').appendChild(wrap);
      requestAnimationFrame(() => wrap.classList.add('show'));
      let closed = false;
      const api = {
        el: wrap,
        $: (sel) => wrap.querySelector(sel),
        close() {
          if (closed) return;
          closed = true;
          const i = openModals.indexOf(api);
          if (i >= 0) openModals.splice(i, 1);
          wrap.classList.remove('show');
          setTimeout(() => wrap.remove(), 220);
          if (opts.onClose) opts.onClose();
        },
        error(message) {
          const box = wrap.querySelector('[data-error]');
          if (box) { box.innerHTML = message ? '<div class="form-error">' + esc(message) + '</div>' : ''; }
        },
      };
      wrap.addEventListener('mousedown', (e) => { if (e.target === wrap && !opts.persistent) api.close(); });
      wrap.addEventListener('click', (e) => {
        if (e.target.closest('[data-close]')) { api.close(); return; }
        const a = e.target.closest('[data-action]');
        if (a && opts.handlers && opts.handlers[a.dataset.action]) { e.preventDefault(); opts.handlers[a.dataset.action](a, e, api); }
      });
      wrap.addEventListener('change', (e) => { if (opts.onChange) opts.onChange(e, api); });
      wrap.addEventListener('submit', (e) => { e.preventDefault(); if (opts.onSubmit) opts.onSubmit(e.target, api); });
      openModals.push(api);
      const first = wrap.querySelector('input:not([disabled]):not([type=hidden]), select:not([disabled])');
      if (first && first.focus) setTimeout(() => first.focus(), 60);
      return api;
    },
    closeTop() { const top = openModals[openModals.length - 1]; if (top) top.close(); },
  };

  function confirmDialog(opts) {
    return new Promise((resolve) => {
      let answered = false;
      Modal.open({
        title: opts.title, size: 'narrow', tone: opts.danger ? 'red' : (opts.tone || 'blue'), icon: opts.icon || (opts.danger ? 'alert' : 'check'),
        body: (opts.message ? '<p style="margin:6px 0 10px">' + esc(opts.message) + '</p>' : '') + (opts.bodyHtml || ''),
        footer: '<button class="btn btn-ghost" data-close>Cancel</button><button class="btn ' + (opts.danger ? 'btn-danger' : 'btn-primary') + '" data-action="ok">' + esc(opts.confirmText || 'Confirm') + '</button>',
        handlers: { ok: (b, e, m) => { answered = true; resolve(true); m.close(); } },
        onClose: () => { if (!answered) resolve(false); },
      });
    });
  }

  // ---- animated counters & charts ----------------------------------------------------------
  function animateCounters(scope) {
    (scope || document).querySelectorAll('[data-count]').forEach((el) => {
      const target = parseFloat(el.dataset.count) || 0, dec = parseInt(el.dataset.decimals || '0', 10), prefix = el.dataset.prefix || '';
      const start = performance.now(), dur = 700;
      const step = (now) => {
        const t = Math.min(1, (now - start) / dur), eased = 1 - Math.pow(1 - t, 3);
        el.textContent = prefix + (target * eased).toFixed(dec);
        if (t < 1) requestAnimationFrame(step);
      };
      requestAnimationFrame(step);
    });
  }

  function donutChart(items, centerNum, centerLabel) {
    const total = items.reduce((a, i) => a + i.value, 0);
    let acc = 0;
    const arcs = total === 0
      ? '<circle cx="21" cy="21" r="15.9155" fill="none" stroke="#e8edf6" stroke-width="5"/>'
      : items.filter((i) => i.value > 0).map((i) => {
        const pct = (i.value / total) * 100;
        const c = '<circle cx="21" cy="21" r="15.9155" fill="none" stroke="' + i.color + '" stroke-width="5" stroke-dasharray="' + pct.toFixed(3) + ' ' + (100 - pct).toFixed(3) + '" stroke-dashoffset="' + (-acc).toFixed(3) + '" transform="rotate(-90 21 21)"/>';
        acc += pct;
        return c;
      }).join('');
    return '<svg class="donut" viewBox="0 0 42 42" role="img" aria-label="' + esc(centerLabel) + ' chart">' + arcs +
      '<text x="21" y="22.4" text-anchor="middle" class="center-num">' + esc(centerNum) + '</text><text x="21" y="27.4" text-anchor="middle" class="center-lbl">' + esc(centerLabel) + '</text></svg>';
  }
  const legendHtml = (items) => '<div class="legend">' + items.map((i) => '<div class="legend-row"><span class="legend-dot" style="background:' + i.color + '"></span><span>' + esc(i.label) + '</span><span class="n">' + i.value + '</span></div>').join('') + '</div>';
  function barChart(items) {
    const max = Math.max.apply(null, items.map((i) => i.value).concat([1]));
    return '<div class="bars">' + items.map((i) => '<div class="bar-col"><span class="bar-val">' + i.value + '</span><div class="bar ' + (i.tone || '') + '" style="height:' + Math.round((i.value / max) * 72) + '%"></div><span class="bar-lbl">' + esc(i.label) + '</span></div>').join('') + '</div>';
  }
  const TYPE_COLORS = { BOOK: '#2f6bff', EBOOK: '#14b8e6', JOURNAL: '#7c5cff' };

  // ---- shared queue visualisation ------------------------------------------------------------
  function queueHtml(entry, compact) {
    let html = '';
    (entry.ready || []).forEach((r) => {
      html += '<div class="q-ready">' + icon('check', 18) + '<span>Next member is ready: ' + esc(r.memberName) + ' — copy held for pickup</span></div>';
    });
    if (!entry.queue.length) return html + (entry.ready && entry.ready.length ? '' : '<div class="q-empty">Nobody is waiting yet.</div>');
    const rows = entry.queue.map((q, i) => '<div class="q-item"><span class="q-pos">' + (i + 1) + '</span><span class="who">' + esc(q.memberName) + '</span><span class="when">since ' + fmtDate(q.reservationDate) + '</span></div>');
    return html + '<div class="queue-line">' + (compact ? rows.slice(0, 3) : rows).join('') + (compact && rows.length > 3 ? '<div class="q-empty">+ ' + (rows.length - 3) + ' more waiting</div>' : '') + '</div>';
  }

  // ---- app / router -------------------------------------------------------------------------------
  const Pages = {};
  const ROUTES = [
    { name: 'dashboard', label: 'Dashboard', icon: 'grid', title: 'Dashboard' },
    { name: 'resources', label: 'Resources', icon: 'book', title: 'Library Resources' },
    { name: 'members', label: 'Members', icon: 'users', title: 'Members' },
    { name: 'loans', label: 'Loans', icon: 'repeat', title: 'Borrow & Return' },
    { name: 'reservations', label: 'Reservations', icon: 'bookmark', title: 'Reservation Queue' },
    { name: 'search', label: 'Search', icon: 'search', title: 'Search' },
    { name: 'reports', label: 'Reports', icon: 'chart', title: 'Reports & Analytics' },
    { name: 'settings', label: 'Settings', icon: 'settings', title: 'Settings' },
  ];
  let renderToken = 0;

  const App = {
    guard() { const mine = renderToken; return () => mine === renderToken; },

    delegate(scope, handlers) {
      scope.onclick = (e) => {
        const el = e.target.closest('[data-action]');
        if (!el || !scope.contains(el) || el.disabled) return;
        const fn = handlers[el.dataset.action];
        if (fn) { e.preventDefault(); fn(el, e); }
      };
    },

    // run an async action; show errors as toasts
    async run(fn, button) {
      if (button) button.disabled = true;
      try { return await fn(); }
      catch (e) { Toast.show(e.message || String(e), 'error'); return undefined; }
      finally { if (button) button.disabled = false; }
    },

    async syncMeta() {
      const pill = $('#backendStatus');
      try {
        const h = (await API.health()).data;
        pill.className = 'status-pill ok';
        $('.status-text', pill).textContent = 'Backend online';
        $('#demoBadge').hidden = !h.demoData;
        const banner = $('#demoBanner');
        banner.hidden = !h.demoData;
        banner.innerHTML = '<strong>DEMO / SAMPLE DATA.</strong> The records you see were generated for demonstration. Settings → “Reset demo data” restores them at any time.';
      } catch (e) {
        pill.className = 'status-pill down';
        $('.status-text', pill).textContent = 'Backend offline';
      }
    },

    async route() {
      const view = $('#view');
      const hash = location.hash.replace(/^#\/?/, '') || 'dashboard';
      const parts = hash.split('/');
      const def = ROUTES.find((r) => r.name === parts[0]) || ROUTES[0];
      const token = ++renderToken;
      document.querySelectorAll('#nav a').forEach((a) => a.classList.toggle('active', a.dataset.route === def.name));
      $('#pageTitle').textContent = def.title;
      document.title = def.title + ' · Smart Library';
      $('#app').classList.remove('drawer-open');
      view.onclick = view.oninput = view.onchange = view.onsubmit = null;
      view.innerHTML = loadingHtml();
      window.scrollTo(0, 0);
      try {
        await Pages[def.name].render(view, parts[1]);
      } catch (e) {
        if (token !== renderToken) return;
        view.innerHTML = errorHtml(e.message);
        App.delegate(view, { retry: () => App.route() });
      }
      if (token === renderToken) App.syncMeta();
    },

    refresh() { return App.route(); },

    boot() {
      $('#nav').innerHTML = ROUTES.map((r) => '<a href="#/' + r.name + '" data-route="' + r.name + '">' + icon(r.icon, 20) + '<span class="label">' + r.label + '</span></a>').join('');
      $('#menuBtn').innerHTML = icon('menu', 22);
      $('#topSave').innerHTML = icon('save', 16) + ' Save state';
      $('#menuBtn').addEventListener('click', () => $('#app').classList.add('drawer-open'));
      $('#scrim').addEventListener('click', () => $('#app').classList.remove('drawer-open'));
      $('#topSave').addEventListener('click', () => App.run(async () => { const r = await API.saveData(); Toast.show(r.message, 'success'); App.syncMeta(); }, $('#topSave')));
      document.addEventListener('keydown', (e) => { if (e.key === 'Escape') Modal.closeTop(); });
      window.addEventListener('hashchange', () => App.route());
      App.route();
    },
  };

  // ---- shared user flows (used by several pages) --------------------------------------------------
  const Flows = {
    // modal with a member <select>; `run(memberId)` performs the API call
    async pickMember(opts) {
      const members = (await API.members()).data;
      const options = members.map((m) => '<option value="' + m.id + '">#' + m.id + ' · ' + esc(m.fullName) + ' (' + m.memberType + ', ' + m.status + ', ' + m.activeLoans + '/' + m.borrowLimit + ' loans)</option>').join('');
      Modal.open({
        title: opts.title, size: 'narrow', tone: opts.tone || 'blue', icon: opts.icon || 'repeat',
        body: '<p style="margin:4px 0 14px;color:var(--muted)">' + esc(opts.intro) + '</p><div data-error></div><div class="field"><label for="pickMember">Member</label><select class="input" id="pickMember">' + options + '</select></div>',
        footer: '<button class="btn btn-ghost" data-close>Cancel</button><button class="btn btn-primary" data-action="go">' + esc(opts.confirmText) + '</button>',
        handlers: {
          go: async (btn, e, modal) => {
            btn.disabled = true; modal.error('');
            try { await opts.run(parseInt(modal.$('#pickMember').value, 10)); modal.close(); }
            catch (err) { modal.error(err.message); btn.disabled = false; }
          },
        },
      });
    },

    borrow(resource) {
      return App.run(() => Flows.pickMember({
        title: 'Borrow resource', intro: '“' + resource.title + '” — ' + resource.availableCopies + ' of ' + resource.totalCopies + ' copies available. The backend validates the member, borrowing limit and availability.',
        confirmText: 'Confirm borrow',
        run: async (memberId) => { const r = await API.borrow(memberId, resource.id); Toast.show(r.message + ' Due ' + fmtDate(r.data.dueDate) + '.', 'success'); App.refresh(); },
      }));
    },

    reserve(resource) {
      return App.run(() => Flows.pickMember({
        title: 'Reserve resource', tone: 'cyan', icon: 'bookmark', intro: '“' + resource.title + '” — members are served first-come, first-served when a copy comes back.',
        confirmText: 'Confirm reservation',
        run: async (memberId) => { const r = await API.reserve(memberId, resource.id); Toast.show(r.message, 'success'); App.refresh(); },
      }));
    },

    async returnLoan(loanId) {
      return App.run(async () => {
        const p = (await API.previewReturn(loanId)).data;
        const body = '<div class="preview ' + (p.overdueDays > 0 ? 'late' : '') + '">' +
          '<div class="row"><span>Member</span><strong>' + esc(p.memberName) + '</strong></div>' +
          '<div class="row"><span>Resource</span><strong>' + esc(p.resourceTitle) + '</strong></div>' +
          '<div class="row"><span>Due date</span><strong>' + fmtDate(p.dueDate) + '</strong></div>' +
          '<div class="row"><span>Returning on</span><strong>' + fmtDate(p.asOfDate) + '</strong></div>' +
          '<div class="row"><span>Overdue days</span><strong>' + p.overdueDays + '</strong></div>' +
          '<div class="row"><span>Fine policy</span><span>' + esc(p.finePolicy) + '</span></div>' +
          '<div class="row"><span>Calculated fine</span><span class="big">' + money(p.calculatedFine) + '</span></div></div>';
        const ok = await confirmDialog({ title: 'Confirm return · Loan #' + loanId, bodyHtml: body, confirmText: p.calculatedFine > 0 ? 'Return & record fine' : 'Confirm return', tone: p.overdueDays > 0 ? 'red' : 'green', icon: 'repeat' });
        if (!ok) return false;
        const r = await API.returnLoan(loanId);
        Toast.show(r.message, 'success');
        (r.data.notifications || []).forEach((n) => Toast.show(n, 'info'));
        App.refresh();
        return true;
      });
    },

    async payFine(loanId, amount) {
      const ok = await confirmDialog({ title: 'Mark fine as paid', message: 'Record payment of ' + money(amount) + ' for loan #' + loanId + '?', confirmText: 'Mark as paid', icon: 'dollar', tone: 'green' });
      if (!ok) return;
      return App.run(async () => { const r = await API.payFine(loanId); Toast.show(r.message, 'success'); App.refresh(); });
    },

    async cancelReservation(id, label) {
      const ok = await confirmDialog({ title: 'Cancel reservation', message: 'Cancel reservation #' + id + (label ? ' for ' + label : '') + '? The next member in line (if any) moves up.', confirmText: 'Cancel reservation', danger: true });
      if (!ok) return;
      return App.run(async () => { const r = await API.cancelReservation(id); Toast.show(r.message, 'success'); App.refresh(); });
    },
  };

  Object.assign(root, { icon, esc, money, fmtDate, $, typeBadge, statusBadge, debounce, highlight, loadingHtml, emptyHtml, errorHtml, Toast, Modal, confirmDialog,
    animateCounters, donutChart, legendHtml, barChart, TYPE_COLORS, queueHtml, Pages, App, Flows });
})(typeof window !== 'undefined' ? window : globalThis);
