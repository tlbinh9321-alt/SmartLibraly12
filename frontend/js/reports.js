/* Reports & Analytics: every number comes from the C++ ReportService. */
(function () {
  function download(fileName, text, mime) {
    const blob = new Blob([text], { type: mime || 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url; a.download = fileName;
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  }

  Pages.reports = {
    async render(view) {
      const alive = App.guard();
      const s = (await API.analytics()).data;
      if (!alive()) return;

      const card = (label, value, sub, ic, tone, extra) => '<div class="card stat"><div class="stat-ic tone-' + tone + '">' + icon(ic, 22) + '</div><div class="label">' + label + '</div><div class="value" data-count="' + value + '" ' + (extra || '') + '>' + (extra ? '$' + Number(value).toFixed(2) : value) + '</div><div class="sub">' + sub + '</div></div>';
      const types = [{ label: 'Books', value: s.resources.books, color: TYPE_COLORS.BOOK }, { label: 'EBooks', value: s.resources.ebooks, color: TYPE_COLORS.EBOOK }, { label: 'Journals', value: s.resources.journals, color: TYPE_COLORS.JOURNAL }];
      const memberTypes = [{ label: 'Students', value: s.members.students, color: '#2f6bff' }, { label: 'Faculty', value: s.members.faculty, color: '#7c5cff' }];
      const statuses = [{ label: 'Active', value: s.members.active, color: '#11a56a' }, { label: 'Inactive', value: s.members.inactive, color: '#9aa7bf' }, { label: 'Suspended', value: s.members.suspended, color: '#e5484d' }];
      const trend = s.borrowingTrend.map((d) => ({ label: d.date.slice(8) + '/' + d.date.slice(5, 7), value: d.count }));
      const buckets = [{ label: '1–7 days', value: s.overdueBuckets[0], tone: 'amber' }, { label: '8–14 days', value: s.overdueBuckets[1], tone: 'amber' }, { label: '15+ days', value: s.overdueBuckets[2], tone: 'red' }];
      const trendTotal = trend.reduce((a, d) => a + d.value, 0);

      view.innerHTML =
        '<div class="page-head"><div><h2>Reports & Analytics</h2><p>Generated ' + fmtDate(s.generatedOn) + ' by the C++ report service.</p></div><div class="spacer">' +
        '<button class="btn btn-primary" data-action="inventory">' + icon('file', 16) + ' EXPORT INVENTORY REPORT</button>' +
        '<button class="btn btn-ghost" data-action="csv">' + icon('download', 16) + ' EXPORT CSV</button>' +
        '<button class="btn btn-ghost" data-action="save">' + icon('save', 16) + ' SAVE SYSTEM STATE</button></div></div>' +
        '<div class="grid stats" style="grid-template-columns:repeat(5,minmax(0,1fr))" id="repStats">' +
        card('Total resources', s.resources.total, s.copies.total + ' copies', 'layers', 'blue') + card('Total members', s.members.total, s.members.active + ' active', 'users', 'violet') +
        card('Active loans', s.loans.active, s.loans.total + ' loans recorded', 'repeat', 'cyan') + card('Overdue', s.loans.overdue, 'need follow-up', 'alert', 'red') +
        card('Fines', s.fines.outstanding, '$' + s.fines.collected.toFixed(2) + ' collected', 'dollar', 'amber', 'data-prefix="$" data-decimals="2"') + '</div>' +
        '<div class="grid two" style="margin-bottom:18px">' +
        '<div class="card"><div class="card-head"><h2>Resource distribution</h2></div><div class="donut-wrap">' + donutChart(types, String(s.resources.total), 'resources') + legendHtml(types) + '</div></div>' +
        '<div class="card"><div class="card-head"><h2>Member distribution</h2><span class="sub">by type and status</span></div><div class="donut-wrap">' + donutChart(memberTypes, String(s.members.total), 'members') + '<div style="flex:1 1 170px;display:flex;flex-direction:column;gap:14px">' + legendHtml(memberTypes) + legendHtml(statuses) + '</div></div></div></div>' +
        '<div class="grid two"><div class="card"><div class="card-head"><h2>Borrowing trends</h2><span class="sub">loans per day, last 14 days · ' + trendTotal + ' total</span></div>' + barChart(trend) + '</div>' +
        '<div class="card"><div class="card-head"><h2>Overdue distribution</h2><span class="sub">open loans past their due date</span></div>' + barChart(buckets) + '</div></div>';
      animateCounters(view);

      App.delegate(view, {
        inventory: (btn) => App.run(async () => {
          const r = (await API.inventoryReport()).data;
          Modal.open({
            title: 'Inventory report', size: 'wide', tone: 'blue', icon: 'file',
            body: '<p class="hint" style="margin-bottom:10px">Saved on the server as <span class="mono">' + esc(r.path) + '</span></p><pre class="report">' + esc(r.content) + '</pre>',
            footer: '<button class="btn btn-ghost" data-close>Close</button><button class="btn btn-primary" data-action="dl">' + icon('download', 16) + ' Download .txt</button>',
            handlers: { dl: () => download(r.fileName, r.content) },
          });
        }, btn),
        csv: (btn) => App.run(async () => {
          const r = (await API.inventoryCsv()).data;
          download(r.fileName, r.content, 'text/csv;charset=utf-8');
          Toast.show('CSV exported (also saved on the server: ' + r.path + ').', 'success');
        }, btn),
        save: (btn) => App.run(async () => { const r = await API.saveData(); Toast.show(r.message, 'success'); }, btn),
      });
    },
  };
})();
