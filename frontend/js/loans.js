/* Borrow & Return page: two big forms + loan list. All validation happens in the C++ backend. */
(function () {
  let statusFilter = 'all';

  Pages.loans = {
    async render(view) {
      view.innerHTML =
        '<div class="page-head"><div><h2>Borrow & Return</h2><p>The backend checks member status, borrowing limit, availability and calculates fines.</p></div></div>' +
        '<div class="grid two" style="margin-bottom:18px">' +
        '<div class="card action-card"><div class="big-ic tone-blue">' + icon('book', 26) + '</div><h3>BORROW RESOURCE</h3><p>Lend a copy to a member.</p>' +
        '<div class="form-grid"><div class="field"><label for="bMember">Member ID</label><input class="input" id="bMember" type="number" min="1" inputmode="numeric" placeholder="e.g. 1"><div class="hint" id="bMemberHint"></div></div>' +
        '<div class="field"><label for="bResource">Resource ID</label><input class="input" id="bResource" type="number" min="1" inputmode="numeric" placeholder="e.g. 2"><div class="hint" id="bResourceHint"></div></div></div>' +
        '<div style="margin-top:16px"><button class="btn btn-primary btn-lg" data-action="borrow">' + icon('repeat', 18) + ' Borrow</button></div></div>' +
        '<div class="card action-card"><div class="big-ic tone-cyan">' + icon('inbox', 26) + '</div><h3>RETURN RESOURCE</h3><p>Check the due date and fine before confirming.</p>' +
        '<div class="field"><label for="rLoan">Loan ID</label><input class="input" id="rLoan" type="number" min="1" inputmode="numeric" placeholder="e.g. 5"></div>' +
        '<div style="margin-top:16px;display:flex;gap:10px;flex-wrap:wrap"><button class="btn btn-cyan btn-lg" data-action="check">' + icon('search', 18) + ' Check loan</button></div><div id="returnPreview"></div></div></div>' +
        '<div class="card"><div class="card-head"><h2>Loans</h2><span class="spacer"></span><div class="chips" id="loanChips"></div></div><div id="loanTable">' + loadingHtml() + '</div></div>';

      const chipDefs = [['all', 'All'], ['active', 'Active'], ['overdue', 'Overdue'], ['returned', 'Returned']];
      const drawChips = () => { view.querySelector('#loanChips').innerHTML = chipDefs.map((c) => '<button class="chip ' + (statusFilter === c[0] ? 'on' : '') + '" data-action="chip" data-v="' + c[0] + '">' + c[1] + '</button>').join(''); };
      drawChips();

      let token = 0;
      const loadLoans = async () => {
        const mine = ++token, box = view.querySelector('#loanTable');
        try {
          const rows = (await API.loans({ status: statusFilter === 'all' ? '' : statusFilter })).data;
          if (mine !== token || !box.isConnected) return;
          box.innerHTML = rows.length
            ? '<div class="table-wrap"><table class="rt"><thead><tr><th>Loan</th><th>Member</th><th>Resource</th><th>Borrowed</th><th>Due</th><th>Returned</th><th>Status</th><th>Fine</th><th style="text-align:right">Actions</th></tr></thead><tbody>' + rows.map((l) => {
              const fine = l.returnDate ? (l.fineAmount > 0 ? money(l.fineAmount) + (l.finePaid ? ' · paid' : ' · unpaid') : '—') : (l.outstandingFine > 0 ? money(l.outstandingFine) + ' · accruing' : '—');
              const action = !l.returnDate ? '<button class="btn btn-sm btn-ghost" data-action="return" data-id="' + l.id + '">' + icon('repeat', 15) + ' Return</button>'
                : (l.fineAmount > 0 && !l.finePaid ? '<button class="btn btn-sm btn-ghost" data-action="pay" data-id="' + l.id + '" data-amount="' + l.fineAmount + '">' + icon('dollar', 15) + ' Pay fine</button>' : '');
              return '<tr><td data-label="Loan" class="num">#' + l.id + '</td><td data-label="Member"><a href="#/members/' + l.memberId + '">' + esc(l.memberName) + '</a></td><td data-label="Resource"><a href="#/resources/' + l.resourceId + '">' + esc(l.resourceTitle) + '</a></td><td data-label="Borrowed">' + fmtDate(l.borrowDate) + '</td><td data-label="Due">' + fmtDate(l.dueDate) + '</td><td data-label="Returned">' + fmtDate(l.returnDate) + '</td><td data-label="Status">' + statusBadge(l.status) + (l.overdueDays > 0 && !l.returnDate ? ' <span class="cell-sub">' + l.overdueDays + 'd</span>' : '') + '</td><td data-label="Fine" class="num">' + fine + '</td><td class="actions">' + action + '</td></tr>';
            }).join('') + '</tbody></table></div><div class="hint" style="padding:10px 22px 16px">' + rows.length + ' loan(s)</div>'
            : emptyHtml('No loans in this view', 'Borrow a resource to create the first loan.', 'repeat');
        } catch (e) { if (mine === token) box.innerHTML = errorHtml(e.message); }
      };
      loadLoans();

      // live name hints (display only — the backend still validates on submit)
      const hint = (id, text, cls) => { const el = view.querySelector(id); el.textContent = text; el.className = 'hint ' + (cls || ''); };
      const lookup = (kind) => debounce(async (e) => {
        const v = e.target.value.trim(), id = kind === 'member' ? '#bMemberHint' : '#bResourceHint';
        if (!v) { hint(id, ''); return; }
        try {
          if (kind === 'member') { const m = (await API.member(v)).data; hint(id, m.fullName + ' · ' + m.memberType + ' · ' + m.status + ' · ' + m.activeLoans + '/' + m.borrowLimit + ' loans', m.status === 'ACTIVE' ? 'good' : 'bad'); }
          else { const r = (await API.resource(v)).data; hint(id, r.title + ' · ' + r.availableCopies + '/' + r.totalCopies + ' available', r.available ? 'good' : 'bad'); }
        } catch (err) { hint(id, err.message, 'bad'); }
      }, 300);
      const memberLookup = lookup('member'), resourceLookup = lookup('resource');
      view.oninput = (e) => { if (e.target.id === 'bMember') memberLookup(e); if (e.target.id === 'bResource') resourceLookup(e); if (e.target.id === 'rLoan') view.querySelector('#returnPreview').innerHTML = ''; };

      const val = (id) => parseInt(view.querySelector(id).value, 10);
      const showPreview = async () => {
        const loanId = val('#rLoan');
        const box = view.querySelector('#returnPreview');
        if (!loanId) { Toast.show('Enter a loan ID first.', 'error'); return; }
        const p = (await API.previewReturn(loanId)).data;
        box.innerHTML = '<div class="preview ' + (p.overdueDays > 0 ? 'late' : '') + '"><div class="row"><span>Member</span><strong>' + esc(p.memberName) + '</strong></div><div class="row"><span>Resource</span><strong>' + esc(p.resourceTitle) + '</strong></div><div class="row"><span>Due date</span><strong>' + fmtDate(p.dueDate) + '</strong></div><div class="row"><span>Overdue days</span><strong>' + p.overdueDays + '</strong></div><div class="row"><span>Fine policy</span><span>' + esc(p.finePolicy) + '</span></div><div class="row"><span>Calculated fine</span><span class="big">' + money(p.calculatedFine) + '</span></div><div style="margin-top:12px"><button class="btn btn-primary" data-action="confirmReturn">Continue to confirm return</button></div></div>';
      };

      App.delegate(view, {
        retry: loadLoans,
        chip: (el) => { statusFilter = el.dataset.v; drawChips(); loadLoans(); },
        borrow: (btn) => App.run(async () => {
          const memberId = val('#bMember'), resourceId = val('#bResource');
          const ok = await confirmDialog({ title: 'Confirm borrow', message: 'Lend resource #' + (resourceId || '?') + ' to member #' + (memberId || '?') + '?', confirmText: 'Borrow', icon: 'repeat' });
          if (!ok) return;
          const r = await API.borrow(memberId || '', resourceId || '');
          Toast.show(r.message + ' Due ' + fmtDate(r.data.dueDate) + '.', 'success');
          App.refresh();
        }, btn),
        check: (btn) => App.run(showPreview, btn),
        confirmReturn: () => Flows.returnLoan(val('#rLoan')),
        'return': (el) => Flows.returnLoan(parseInt(el.dataset.id, 10)),
        pay: (el) => Flows.payFine(parseInt(el.dataset.id, 10), parseFloat(el.dataset.amount)),
      });
    },
  };
})();
