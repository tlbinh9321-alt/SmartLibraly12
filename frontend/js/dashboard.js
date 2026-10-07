/* Dashboard: stat cards, resource overview chart, recent activity, overdue loans, queues. */
Pages.dashboard = {
  async render(view) {
    const alive = App.guard();
    const data = (await API.dashboard()).data;
    if (!alive()) return;
    const s = data.stats;

    const card = (label, value, sub, ic, tone, extra) =>
      '<div class="card stat"><div class="stat-ic tone-' + tone + '">' + icon(ic, 22) + '</div><div class="label">' + label + '</div>' +
      '<div class="value" data-count="' + value + '" ' + (extra || '') + '>' + (extra && extra.includes('$') ? '$' + Number(value).toFixed(2) : value) + '</div><div class="sub">' + sub + '</div></div>';

    const typeItems = [
      { label: 'Books', value: s.resources.books, color: TYPE_COLORS.BOOK },
      { label: 'EBooks', value: s.resources.ebooks, color: TYPE_COLORS.EBOOK },
      { label: 'Journals', value: s.resources.journals, color: TYPE_COLORS.JOURNAL },
    ];
    const pctBorrowed = s.copies.total ? Math.round((s.copies.borrowed / s.copies.total) * 100) : 0;

    const recent = data.recentLoans.length
      ? '<div class="table-wrap"><table class="rt"><thead><tr><th>Loan</th><th>Member</th><th>Resource</th><th>Borrowed</th><th>Status</th></tr></thead><tbody>' +
        data.recentLoans.map((l) => '<tr><td data-label="Loan" class="num">#' + l.id + '</td><td data-label="Member">' + esc(l.memberName) + '</td><td data-label="Resource"><span class="cell-title">' + esc(l.resourceTitle) + '</span></td><td data-label="Borrowed">' + fmtDate(l.borrowDate) + '</td><td data-label="Status">' + statusBadge(l.status) + '</td></tr>').join('') + '</tbody></table></div>'
      : emptyHtml('No loans yet', 'Borrowed resources will show up here.', 'repeat');

    const overdue = data.overdueLoans.length
      ? '<div class="table-wrap"><table class="rt"><thead><tr><th>Member</th><th>Resource</th><th>Due</th><th>Days late</th><th>Fine so far</th><th></th></tr></thead><tbody>' +
        data.overdueLoans.map((l) => '<tr><td data-label="Member">' + esc(l.memberName) + '</td><td data-label="Resource"><span class="cell-title">' + esc(l.resourceTitle) + '</span></td><td data-label="Due">' + fmtDate(l.dueDate) + '</td><td data-label="Days late"><span class="badge st-overdue">' + l.overdueDays + ' days</span></td><td data-label="Fine so far" class="num">' + money(l.outstandingFine) + '</td><td class="actions"><button class="btn btn-sm btn-ghost" data-action="return" data-id="' + l.id + '">' + icon('repeat', 15) + ' Return</button></td></tr>').join('') + '</tbody></table></div>'
      : emptyHtml('Nothing overdue', 'Every loan is within its due date.', 'check');

    const queues = data.queues.filter((q) => q.queue.length || q.ready.length);
    const queueCards = queues.length
      ? '<div class="grid two" style="padding:14px 22px 22px">' + queues.slice(0, 4).map((q) =>
        '<div class="card queue-card" style="box-shadow:none"><div class="queue-top"><h3>' + esc(q.resource.title) + '</h3><span style="margin-left:auto">' + statusBadge(q.resource.availability) + '</span></div>' + queueHtml(q, true) + '</div>').join('') + '</div>'
      : emptyHtml('No reservation queues', 'Queues appear when members reserve unavailable resources.', 'bookmark');

    view.innerHTML =
      '<div class="grid stats">' +
      card('Total resources', s.resources.total, s.copies.total + ' copies in total', 'layers', 'blue') +
      card('Available', s.copies.available, 'copies on the shelf', 'check', 'green') +
      card('Borrowed', s.copies.borrowed, pctBorrowed + '% of all copies', 'repeat', 'cyan') +
      card('Overdue', s.loans.overdue, 'loans past due date', 'alert', 'red') + '</div>' +
      '<div class="grid stats">' +
      card('Members', s.members.total, s.members.students + ' students · ' + s.members.faculty + ' faculty', 'users', 'violet') +
      card('Active loans', s.loans.active, s.loans.returned + ' already returned', 'clock', 'blue') +
      card('Reservations', s.reservations.waiting + s.reservations.ready, s.reservations.ready + ' ready for pickup', 'bookmark', 'amber') +
      card('Outstanding fines', s.fines.outstanding, money(s.fines.collected) + ' collected', 'dollar', 'red', 'data-prefix="$" data-decimals="2"') + '</div>' +
      '<div class="grid hero" style="margin-bottom:18px">' +
      '<div class="card"><div class="card-head"><h2>Resource overview</h2><span class="sub">by type</span></div><div class="donut-wrap">' +
      donutChart(typeItems, String(s.resources.total), 'resources') + '<div class="legend" style="flex:1 1 180px">' +
      typeItems.map((i) => '<div class="legend-row"><span class="legend-dot" style="background:' + i.color + '"></span><span>' + i.label + '</span><span class="n">' + i.value + '</span></div>').join('') +
      '<div style="margin-top:6px"><div class="hint" style="display:flex;justify-content:space-between"><span>Copies in circulation</span><strong>' + s.copies.borrowed + ' / ' + s.copies.total + '</strong></div><div class="meter"><span style="width:' + pctBorrowed + '%;background:linear-gradient(90deg,var(--cyan),var(--blue))"></span></div></div></div></div></div>' +
      '<div class="card"><div class="card-head"><h2>Recent borrowing activity</h2><span class="spacer"></span><a href="#/loans">All loans</a></div>' + recent + '</div></div>' +
      '<div class="card" style="margin-bottom:18px"><div class="card-head"><h2>Overdue loans</h2><span class="sub">fines accrue until the resource is returned</span></div>' + overdue + '</div>' +
      '<div class="card"><div class="card-head"><h2>Reservation queue</h2><span class="spacer"></span><a href="#/reservations">Manage queues</a></div>' + queueCards + '</div>';

    animateCounters(view);
    App.delegate(view, { 'return': (el) => Flows.returnLoan(parseInt(el.dataset.id, 10)) });
  },
};
