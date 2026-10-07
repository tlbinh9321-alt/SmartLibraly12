/* Members: list with filters, add/edit modal, detail page. */
(function () {
  const filters = { q: '', type: '', status: '' };

  function memberForm(m) {
    const sel = (name, opts, cur) => '<select class="input" name="' + name + '">' + opts.map((o) => '<option ' + (o === cur ? 'selected' : '') + '>' + o + '</option>').join('') + '</select>';
    return '<form id="memForm"><div data-error></div><div class="form-grid">' +
      '<div class="field"><label>Membership type</label>' + sel('memberType', ['STUDENT', 'FACULTY'], m ? m.memberType : 'STUDENT') + '</div>' +
      '<div class="field"><label>Status</label>' + sel('status', ['ACTIVE', 'INACTIVE', 'SUSPENDED'], m ? m.status : 'ACTIVE') + '</div>' +
      '<div class="field full"><label>Full name</label><input class="input" name="fullName" required value="' + esc(m ? m.fullName : '') + '"></div>' +
      '<div class="field"><label>Email</label><input class="input" name="email" type="email" required value="' + esc(m ? m.email : '') + '"></div>' +
      '<div class="field"><label>Phone</label><input class="input" name="phone" value="' + esc(m ? m.phone : '') + '" placeholder="+84 90 123 4567"></div></div>' +
      '<p class="hint">Students: 3 loans / 14 days. Faculty: 10 loans / 30 days with a fine grace period. The backend applies these rules.</p></form>';
  }

  function openForm(m) {
    Modal.open({
      title: m ? 'Edit member #' + m.id : 'Add member',
      body: memberForm(m),
      footer: '<button class="btn btn-ghost" data-close>Cancel</button><button class="btn btn-primary" data-action="save">' + (m ? 'Save changes' : 'Add member') + '</button>',
      onSubmit: (f, modal) => modal.$('[data-action=save]').click(),
      handlers: {
        save: async (btn, e, modal) => {
          btn.disabled = true; modal.error('');
          const body = {};
          new FormData(modal.$('#memForm')).forEach((v, k) => { body[k] = v; });
          try {
            const r = m ? await API.updateMember(m.id, body) : await API.createMember(body);
            Toast.show(r.message, 'success'); modal.close(); App.refresh();
          } catch (err) { modal.error(err.message); btn.disabled = false; }
        },
      },
    });
  }

  Pages.members = {
    async render(view, id) {
      if (id) return Pages.members.renderDetail(view, parseInt(id, 10));
      let list = [], token = 0;
      const opts = (arr, cur, all) => ['<option value="">' + all + '</option>'].concat(arr.map((o) => '<option ' + (o === cur ? 'selected' : '') + '>' + o + '</option>')).join('');
      view.innerHTML =
        '<div class="page-head"><div><h2>Members</h2><p>Students and faculty with their borrowing limits and fines.</p></div><div class="spacer"><button class="btn btn-primary" data-action="add">' + icon('plus', 16) + ' Add Member</button></div></div>' +
        '<div class="card"><div class="filters"><div class="search-box">' + icon('search', 18) + '<input class="input" id="mq" placeholder="Search name, email or member ID…" value="' + esc(filters.q) + '"></div>' +
        '<select class="input" id="mtype">' + opts(['STUDENT', 'FACULTY'], filters.type, 'All types') + '</select>' +
        '<select class="input" id="mstatus">' + opts(['ACTIVE', 'INACTIVE', 'SUSPENDED'], filters.status, 'Any status') + '</select></div><div id="memTable">' + loadingHtml() + '</div></div>';

      const load = async () => {
        const mine = ++token, box = view.querySelector('#memTable');
        try {
          const res = await API.members(filters);
          if (mine !== token || !box.isConnected) return;
          list = res.data;
          box.innerHTML = list.length
            ? '<div class="table-wrap"><table class="rt"><thead><tr><th>Member ID</th><th>Name</th><th>Type</th><th>Email</th><th>Status</th><th>Borrowed</th><th>Fine</th><th style="text-align:right">Actions</th></tr></thead><tbody>' +
              list.map((m) => '<tr><td data-label="Member ID" class="num">#' + m.id + '</td><td data-label="Name" class="cell-main"><button class="link-btn" data-action="view" data-id="' + m.id + '">' + esc(m.fullName) + '</button></td><td data-label="Type">' + typeBadge(m.memberType) + '</td><td data-label="Email">' + esc(m.email) + '</td><td data-label="Status">' + statusBadge(m.status) + '</td><td data-label="Borrowed" class="num">' + m.activeLoans + ' / ' + m.borrowLimit + '</td><td data-label="Fine" class="num" style="' + (m.outstandingFines > 0 ? 'color:var(--red);font-weight:700' : '') + '">' + money(m.outstandingFines) + '</td><td class="actions"><button class="icon-btn" title="View" aria-label="View" data-action="view" data-id="' + m.id + '">' + icon('eye') + '</button><button class="icon-btn" title="Edit" aria-label="Edit" data-action="edit" data-id="' + m.id + '">' + icon('edit') + '</button></td></tr>').join('') +
              '</tbody></table></div><div class="hint" style="padding:10px 22px 16px">' + list.length + ' member(s)</div>'
            : emptyHtml('No members match', 'Adjust the filters or add a member.', 'users');
        } catch (e) { if (mine === token) box.innerHTML = errorHtml(e.message); }
      };
      load();
      const byId = (el) => list.find((m) => m.id === parseInt(el.dataset.id, 10));
      view.oninput = debounce((e) => { if (e.target.id === 'mq') { filters.q = e.target.value; load(); } }, 250);
      view.onchange = (e) => { const map = { mtype: 'type', mstatus: 'status' }; if (map[e.target.id]) { filters[map[e.target.id]] = e.target.value; load(); } };
      App.delegate(view, { retry: load, add: () => openForm(null), view: (el) => { location.hash = '#/members/' + el.dataset.id; }, edit: (el) => openForm(byId(el)) });
    },

    async renderDetail(view, id) {
      const alive = App.guard();
      const m = (await API.member(id)).data;
      if (!alive()) return;
      $('#pageTitle').textContent = m.fullName;
      const fact = (k, v) => '<div class="fact"><div class="k">' + k + '</div><div class="v">' + v + '</div></div>';
      const fineText = (l) => (l.returnDate ? (l.fineAmount > 0 ? money(l.fineAmount) + (l.finePaid ? ' · paid' : ' · unpaid') : '—') : (l.outstandingFine > 0 ? money(l.outstandingFine) + ' · accruing' : '—'));
      const loanRow = (l, withReturn) => '<tr><td data-label="Loan">#' + l.id + '</td><td data-label="Resource"><a href="#/resources/' + l.resourceId + '">' + esc(l.resourceTitle) + '</a></td><td data-label="Borrowed">' + fmtDate(l.borrowDate) + '</td><td data-label="Due">' + fmtDate(l.dueDate) + '</td><td data-label="' + (withReturn ? 'Status' : 'Returned') + '">' + (withReturn ? statusBadge(l.status) : fmtDate(l.returnDate)) + '</td><td data-label="Fine" class="num">' + fineText(l) + '</td><td class="actions">' +
        (withReturn ? '<button class="btn btn-sm btn-ghost" data-action="return" data-id="' + l.id + '">' + icon('repeat', 15) + ' Return</button>' : (l.returnDate && l.fineAmount > 0 && !l.finePaid ? '<button class="btn btn-sm btn-ghost" data-action="pay" data-id="' + l.id + '" data-amount="' + l.fineAmount + '">' + icon('dollar', 15) + ' Pay fine</button>' : '')) + '</td></tr>';
      const loanTable = (rows, withReturn, empty) => rows.length
        ? '<div class="table-wrap"><table class="rt"><thead><tr><th>Loan</th><th>Resource</th><th>Borrowed</th><th>Due</th><th>' + (withReturn ? 'Status' : 'Returned') + '</th><th>Fine</th><th></th></tr></thead><tbody>' + rows.map((l) => loanRow(l, withReturn)).join('') + '</tbody></table></div>'
        : emptyHtml(empty, '', 'repeat');
      const past = m.loanHistory.filter((l) => l.status === 'RETURNED');
      const res = m.reservations.length
        ? '<div class="table-wrap"><table class="rt"><thead><tr><th>#</th><th>Resource</th><th>Reserved</th><th>Status</th><th>Position</th><th></th></tr></thead><tbody>' + m.reservations.map((r) => '<tr><td data-label="#">' + r.id + '</td><td data-label="Resource"><a href="#/resources/' + r.resourceId + '">' + esc(r.resourceTitle) + '</a></td><td data-label="Reserved">' + fmtDate(r.reservationDate) + '</td><td data-label="Status">' + statusBadge(r.status) + '</td><td data-label="Position">' + (r.position || '—') + '</td><td class="actions">' + (r.status === 'WAITING' || r.status === 'READY' ? '<button class="btn btn-sm btn-soft-danger" data-action="cancel" data-id="' + r.id + '">Cancel</button>' : '') + '</td></tr>').join('') + '</tbody></table></div>'
        : emptyHtml('No reservations', '', 'bookmark');
      view.innerHTML =
        '<a class="back-link" href="#/members">' + icon('back', 16) + ' All members</a>' +
        '<div class="card" style="margin-bottom:18px"><div class="card-pad" style="display:flex;gap:14px;flex-wrap:wrap;align-items:flex-start"><div style="flex:1 1 300px"><div style="display:flex;gap:8px;margin-bottom:8px">' + typeBadge(m.memberType) + statusBadge(m.status) + '</div><h2 style="font-size:26px">' + esc(m.fullName) + '</h2><p style="color:var(--muted);margin:4px 0 0">Member #' + m.id + '</p></div><button class="btn btn-ghost" data-action="edit">' + icon('edit', 16) + ' Edit profile</button></div>' +
        '<div class="facts">' + fact('Email', esc(m.email)) + fact('Phone', esc(m.phone || '—')) + fact('Registered', fmtDate(m.registrationDate)) + fact('Borrow limit', m.activeLoans + ' of ' + m.borrowLimit + ' used') + fact('Loan period', m.loanDurationDays + ' days') + fact('Outstanding fines', '<span style="color:' + (m.outstandingFines > 0 ? 'var(--red)' : 'var(--green)') + '">' + money(m.outstandingFines) + '</span>') + '</div></div>' +
        '<div class="stack"><div class="card"><div class="card-head"><h2>Current loans</h2></div>' + loanTable(m.currentLoans, true, 'No active loans') + '</div>' +
        '<div class="card"><div class="card-head"><h2>Borrow history</h2><span class="sub">returned loans and fines</span></div>' + loanTable(past, false, 'No returned loans yet') + '</div>' +
        '<div class="card"><div class="card-head"><h2>Reservations</h2></div>' + res + '</div></div>';
      App.delegate(view, {
        edit: () => openForm(m),
        'return': (el) => Flows.returnLoan(parseInt(el.dataset.id, 10)),
        pay: (el) => Flows.payFine(parseInt(el.dataset.id, 10), parseFloat(el.dataset.amount)),
        cancel: (el) => Flows.cancelReservation(parseInt(el.dataset.id, 10), m.fullName),
      });
    },
  };
})();
