/* Settings: policy overview (read from the backend), persistence controls, safe reset. */
Pages.settings = {
  async render(view) {
    const alive = App.guard();
    const s = (await API.settings()).data;
    if (!alive()) return;
    const kb = (b) => (b / 1024).toFixed(1) + ' KB';

    view.innerHTML =
      '<div class="page-head"><div><h2>Settings</h2><p>Policies are defined in the C++ domain classes; this page shows what the backend applies.</p></div></div>' +
      '<div class="grid two" style="margin-bottom:18px">' +
      '<div class="card card-pad"><h2 style="margin-bottom:12px">Library information</h2><form id="libForm"><div class="field"><label for="libName">Library name</label><input class="input" id="libName" value="' + esc(s.libraryName) + '" maxlength="80"></div>' +
      '<div class="kv" style="margin-top:12px"><span class="k">Data type</span><span>' + (s.demoData ? '<span class="badge demo-badge">DEMO / SAMPLE DATA</span>' : 'Your own data') + '</span><span class="k">Today</span><span>' + fmtDate(s.status.today) + '</span></div>' +
      '<div style="margin-top:12px"><button class="btn btn-primary btn-sm" type="submit">Save name</button></div></form></div>' +
      '<div class="card card-pad"><h2 style="margin-bottom:6px">System status</h2><div class="kv"><span class="k">Resources</span><strong>' + s.status.resources + '</strong><span class="k">Members</span><strong>' + s.status.members + '</strong><span class="k">Loans</span><strong>' + s.status.loans + '</strong><span class="k">Reservations</span><strong>' + s.status.reservations + '</strong><span class="k">Reports folder</span><span class="mono" style="word-break:break-all">' + esc(s.status.reportsDirectory) + '</span></div></div></div>' +
      '<div class="grid two" style="margin-bottom:18px">' +
      '<div class="card"><div class="card-head"><h2>Borrowing policies</h2></div><div class="table-wrap"><table class="rt"><thead><tr><th>Member type</th><th>Borrow limit</th><th>Loan period</th></tr></thead><tbody>' + s.borrowingPolicies.map((p) => '<tr><td data-label="Member type">' + typeBadge(p.memberType) + '</td><td data-label="Borrow limit">' + p.borrowLimit + ' resources</td><td data-label="Loan period">' + p.loanDays + ' days</td></tr>').join('') + '</tbody></table></div></div>' +
      '<div class="card"><div class="card-head"><h2>Fine rules</h2></div><div class="table-wrap"><table class="rt"><thead><tr><th>Resource</th><th>Rate per day</th></tr></thead><tbody>' + s.fineRules.resourceRates.map((r) => '<tr><td data-label="Resource">' + typeBadge(r.type) + '</td><td data-label="Rate per day" class="num">' + money(r.ratePerDay) + '</td></tr>').join('') + '</tbody></table></div>' +
      '<div style="padding:0 22px 18px">' + s.fineRules.memberRules.map((r) => '<div class="hint" style="margin-top:8px">' + typeBadge(r.memberType) + ' &nbsp;×' + r.rateFactor + ' rate, ' + r.graceDays + ' grace day(s) — ' + esc(r.description) + '</div>').join('') + '<div class="hint" style="margin-top:10px"><em>' + esc(s.fineRules.formula) + '</em></div></div></div></div>' +
      '<div class="card" style="margin-bottom:18px"><div class="card-head"><h2>Persistence</h2><span class="sub">auto-saved after every change</span></div><div style="padding:14px 22px 6px"><div class="kv"><span class="k">Data folder</span><span class="mono" style="word-break:break-all">' + esc(s.persistence.directory) + '</span><span class="k">Format</span><span>' + esc(s.persistence.format) + '</span><span class="k">Last manual save</span><span>' + esc(s.persistence.lastSavedAt || 'since server start: none (auto-save is on)') + '</span></div></div>' +
      '<div class="table-wrap"><table class="rt"><thead><tr><th>File</th><th>Present</th><th>Size</th></tr></thead><tbody>' + s.persistence.files.map((f) => '<tr><td data-label="File" class="mono">' + esc(f.name) + '</td><td data-label="Present">' + (f.exists ? statusBadge('ACTIVE') : statusBadge('INACTIVE')) + '</td><td data-label="Size" class="num">' + (f.exists ? kb(f.bytes) : '—') + '</td></tr>').join('') + '</tbody></table></div>' +
      '<div style="display:flex;gap:10px;flex-wrap:wrap;padding:6px 22px 22px"><button class="btn btn-primary" data-action="save">' + icon('save', 16) + ' SAVE DATA</button><button class="btn btn-ghost" data-action="load">' + icon('upload', 16) + ' LOAD DATA</button><button class="btn btn-ghost" data-action="report">' + icon('file', 16) + ' EXPORT REPORT</button><button class="btn btn-soft-danger" data-action="reset">' + icon('refresh', 16) + ' RESET DEMO DATA</button></div></div>';

    view.onsubmit = (e) => {
      e.preventDefault();
      App.run(async () => { const r = await API.saveSettings({ libraryName: view.querySelector('#libName').value }); Toast.show(r.message, 'success'); });
    };
    App.delegate(view, {
      save: (b) => App.run(async () => { const r = await API.saveData(); Toast.show(r.message, 'success'); App.refresh(); }, b),
      load: async (b) => {
        const ok = await confirmDialog({ title: 'Load data from files', message: 'This replaces the data currently in memory with what is stored in the data files.', confirmText: 'Load data', tone: 'amber', icon: 'upload' });
        if (ok) App.run(async () => { const r = await API.loadData(); Toast.show(r.message, 'success'); App.refresh(); }, b);
      },
      report: (b) => App.run(async () => { const r = await API.inventoryReport(); Toast.show('Report written to ' + r.data.path, 'success'); }, b),
      reset: () => {
        Modal.open({
          title: 'Reset demo data', size: 'narrow', tone: 'red', icon: 'alert',
          body: '<div data-error></div><p style="margin:4px 0 12px"><strong>This permanently deletes ALL current resources, members, loans and reservations</strong> and replaces them with the built-in demo data.</p><div class="field"><label for="resetWord">Type <strong>RESET</strong> to confirm</label><input class="input" id="resetWord" autocomplete="off"></div>',
          footer: '<button class="btn btn-ghost" data-close>Cancel</button><button class="btn btn-danger" data-action="go" disabled>Reset everything</button>',
          onChange: (e, m) => { m.$('[data-action=go]').disabled = m.$('#resetWord').value.trim() !== 'RESET'; },
          handlers: {
            go: async (b, e, m) => {
              if (m.$('#resetWord').value.trim() !== 'RESET') return;
              b.disabled = true;
              try { const r = await API.resetDemo(); Toast.show(r.message, 'success'); m.close(); App.refresh(); }
              catch (err) { m.error(err.message); b.disabled = false; }
            },
          },
        });
        const m = document.querySelector('#resetWord');
        if (m) m.addEventListener('input', () => { const btn = document.querySelector('.modal [data-action=go]'); if (btn) btn.disabled = m.value.trim() !== 'RESET'; });
      },
    });
  },
};
