/* Reservation queues: one card per resource that is unavailable or has a queue. */
Pages.reservations = {
  async render(view) {
    const alive = App.guard();
    const [queues, all] = await Promise.all([API.queues(), API.reservations()]);
    if (!alive()) return;
    const list = queues.data, rows = all.data;

    const cards = list.length
      ? '<div class="grid two">' + list.map((q) => {
        const r = q.resource;
        return '<div class="card queue-card"><div class="queue-top"><div style="min-width:0"><h3>' + esc(r.title) + '</h3><div class="cell-sub">' + esc(r.author) + ' · ' + r.availableCopies + '/' + r.totalCopies + ' copies on shelf</div></div><span style="margin-left:auto;display:flex;gap:6px;flex-wrap:wrap">' + typeBadge(r.type) + statusBadge(r.availability) + '</span></div>' +
          '<div style="margin-top:12px;font-weight:700;font-size:12.5px;letter-spacing:.06em;color:var(--muted)">WAITING QUEUE</div>' + queueHtml(q, true) +
          '<div style="display:flex;gap:8px;flex-wrap:wrap"><button class="btn btn-sm btn-ghost" data-action="viewq" data-id="' + r.id + '">View Queue</button>' + (r.available ? '' : '<button class="btn btn-sm btn-cyan" data-action="reserve" data-id="' + r.id + '">' + icon('plus', 14) + ' Reserve</button>') + '<a class="btn btn-sm btn-ghost" href="#/resources/' + r.id + '">Resource</a></div></div>';
      }).join('') + '</div>'
      : '<div class="card">' + emptyHtml('Every resource is available', 'Queues appear when a resource has no free copy.', 'check') + '</div>';

    const table = rows.length
      ? '<div class="table-wrap"><table class="rt"><thead><tr><th>#</th><th>Member</th><th>Resource</th><th>Reserved</th><th>Status</th><th>Position</th><th></th></tr></thead><tbody>' + rows.slice().reverse().map((r) => '<tr><td data-label="#" class="num">' + r.id + '</td><td data-label="Member"><a href="#/members/' + r.memberId + '">' + esc(r.memberName) + '</a></td><td data-label="Resource"><a href="#/resources/' + r.resourceId + '">' + esc(r.resourceTitle) + '</a></td><td data-label="Reserved">' + fmtDate(r.reservationDate) + '</td><td data-label="Status">' + statusBadge(r.status) + '</td><td data-label="Position" class="num">' + (r.position || '—') + '</td><td class="actions">' + (r.status === 'WAITING' || r.status === 'READY' ? '<button class="btn btn-sm btn-soft-danger" data-action="cancel" data-id="' + r.id + '" data-name="' + esc(r.memberName) + '">Cancel</button>' : '') + '</td></tr>').join('') + '</tbody></table></div>'
      : emptyHtml('No reservations yet', '', 'bookmark');

    view.innerHTML =
      '<div class="page-head"><div><h2>Reservation Queue</h2><p>First come, first served. A returned copy is held for the first waiting member.</p></div><div class="spacer"><button class="btn btn-primary" data-action="new">' + icon('plus', 16) + ' New reservation</button></div></div>' +
      cards + '<div class="card" style="margin-top:18px"><div class="card-head"><h2>All reservations</h2></div>' + table + '</div>';

    const find = (el) => list.find((q) => q.resource.id === parseInt(el.dataset.id, 10));
    App.delegate(view, {
      reserve: (el) => Flows.reserve(find(el).resource),
      cancel: (el) => Flows.cancelReservation(parseInt(el.dataset.id, 10), el.dataset.name),
      viewq: (el) => {
        const q = find(el);
        const entries = q.ready.concat(q.queue);
        Modal.open({
          title: 'Queue · ' + q.resource.title, size: 'narrow',
          body: (entries.length ? entries.map((e) => '<div class="q-item" style="margin-bottom:8px"><span class="q-pos" style="' + (e.status === 'READY' ? 'background:var(--cyan)' : '') + '">' + (e.status === 'READY' ? '✓' : e.position) + '</span><span class="who">' + esc(e.memberName) + '</span>' + statusBadge(e.status) + '<button class="btn btn-sm btn-soft-danger" data-action="cancelq" data-id="' + e.id + '" data-name="' + esc(e.memberName) + '">Cancel</button></div>').join('') : '<div class="q-empty">Nobody is waiting.</div>'),
          footer: '<button class="btn btn-ghost" data-close>Close</button>',
          handlers: { cancelq: (b, ev, m) => { m.close(); Flows.cancelReservation(parseInt(b.dataset.id, 10), b.dataset.name); } },
        });
      },
      'new': () => App.run(async () => {
        const resources = (await API.resources({ availability: 'unavailable' })).data;
        if (!resources.length) { Toast.show('Every resource is currently available — borrow instead of reserving.', 'info'); return; }
        const opts = resources.map((r) => '<option value="' + r.id + '">#' + r.id + ' · ' + esc(r.title) + ' (' + r.queueLength + ' waiting)</option>').join('');
        Modal.open({
          title: 'New reservation', size: 'narrow', tone: 'cyan', icon: 'bookmark',
          body: '<div data-error></div><div class="field"><label>Unavailable resource</label><select class="input" id="nrRes">' + opts + '</select></div>',
          footer: '<button class="btn btn-ghost" data-close>Cancel</button><button class="btn btn-primary" data-action="next">Choose member</button>',
          handlers: { next: (b, e, m) => { const r = resources.find((x) => String(x.id) === m.$('#nrRes').value); m.close(); Flows.reserve(r); } },
        });
      }),
    });
  },
};
