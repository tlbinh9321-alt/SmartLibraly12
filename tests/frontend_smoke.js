// Headless frontend smoke test (no browser needed): loads every page module with a stub DOM and renders it
// against the live C++ API. Start the server first:  smartlibrary --port 8099 --data /tmp/sl/data --reports /tmp/sl/reports
// Headless smoke test of the frontend modules: real API calls against the C++ server, stub DOM.
const vm = require('vm'), fs = require('fs'), path = require('path');
const root = path.join(__dirname, '..', 'frontend', 'js') + path.sep;   // run: node tests/frontend_smoke.js (server on :8099 must be running)
function stubEl() {
  const el = { _html: '', _q: {}, dataset: {}, style: {}, value: '', textContent: '', isConnected: true, onclick: null,
    classList: { add() {}, remove() {}, toggle() {}, contains() { return false; } },
    get innerHTML() { return this._html; }, set innerHTML(v) { this._html = v; },
    querySelector(sel) { return (this._q[sel] = this._q[sel] || stubEl()); }, querySelectorAll() { return []; },
    addEventListener() {}, appendChild(c) { return c; }, remove() {}, contains() { return true; }, closest() { return null; },
    focus() {}, click() {}, setAttribute() {}, getAttribute() { return null; } };
  return el;
}
const doc = stubEl(); doc.createElement = () => stubEl(); doc.body = stubEl(); doc.title = '';
const ctx = { document: doc, location: { hash: '' }, fetch, console, setTimeout, clearTimeout, performance,
  requestAnimationFrame: (cb) => setTimeout(() => cb(performance.now()), 0), Blob, URL, FormData: class { forEach() {} } };
ctx.window = ctx; ctx.window.scrollTo = () => {}; ctx.window.addEventListener = () => {};
ctx.SMARTLIB_API_BASE = 'http://127.0.0.1:8099';
vm.createContext(ctx);
['api', 'app', 'dashboard', 'resources', 'members', 'loans', 'reservations', 'search', 'reports', 'settings'].forEach((f) => vm.runInContext(fs.readFileSync(root + f + '.js', 'utf8'), ctx, { filename: f + '.js' }));

const run = (code) => vm.runInContext(code, ctx);
(async () => {
  let failures = 0;
  const check = (name, cond, extra) => { console.log((cond ? 'ok   ' : 'FAIL ') + name + (extra ? '  ' + extra : '')); if (!cond) failures++; };
  const view = ctx.document.querySelector('#view');
  const routes = [['dashboard', ['Total resources', 'Overdue loans', 'Reservation queue', 'Clean Code']], ['resources', ['Library Resources', 'Add Resource']], ['resources/1', ['Clean Code', 'Borrowing history', 'Reservation queue']],
    ['members', ['Add Member', 'Nguyễn Văn A']], ['members/1', ['Current loans', 'Borrow history']], ['loans', ['BORROW RESOURCE', 'RETURN RESOURCE']], ['reservations', ['Reservation Queue', 'WAITING QUEUE']],
    ['search', ['Search books']], ['reports', ['EXPORT INVENTORY REPORT', 'Borrowing trends']], ['settings', ['RESET DEMO DATA', 'Fine rules']]];
  for (const [r, needles] of routes) {
    ctx.location.hash = '#/' + r;
    await run('App.route()');
    await new Promise((res) => setTimeout(res, 400));   // let async table loads finish
    let html = view._html;
    for (const k of Object.keys(view._q)) html += view._q[k]._html;
    const missing = needles.filter((n) => !html.includes(n));
    check('route #/' + r, missing.length === 0 && !html.includes('Something went wrong') && !html.includes('undefined'), missing.length ? 'missing: ' + missing.join(', ') : html.length + ' chars');
  }
  // list pages: table fragments
  ctx.location.hash = '#/resources'; await run('App.route()'); await new Promise((r) => setTimeout(r, 400));
  const t = view._q['#resTable']._html;
  check('resources table rows', (t.match(/<tr>/g) || []).length >= 13, (t.match(/<tr>/g) || []).length + ' rows');
  check('badges differentiate types', t.includes('type-book') && t.includes('type-ebook') && t.includes('type-journal'));
  ctx.location.hash = '#/members'; await run('App.route()'); await new Promise((r) => setTimeout(r, 400));
  check('members table rows', (view._q['#memTable']._html.match(/<tr>/g) || []).length >= 9);
  ctx.location.hash = '#/loans'; await run('App.route()'); await new Promise((r) => setTimeout(r, 400));
  check('loans table has overdue rows', view._q['#loanTable']._html.includes('st-overdue'));
  // search page flow
  ctx.location.hash = '#/search'; await run('App.route()');
  const sq = view.querySelector('#sq'); sq.value = 'harry';
  run('Pages.search')   // ensure registered
  await run("(async()=>{ const r = (await API.search({query:'harry'})).data; globalThis.__sr = r; })()");
  check('search "harry" via API finds Harry Potter', ctx.__sr.count === 1 && ctx.__sr.results[0].title.startsWith('Harry Potter'));
  // modal builders do not throw
  await run("API.resource(4).then(r => Flows.borrow(r.data))"); await run("API.resource(4).then(r => Flows.reserve(r.data))");
  check('borrow/reserve modals build', true);
  // API error mapping
  let msg = ''; try { await run("API.borrow(999, 1)"); } catch (e) { msg = e.message; }
  check('ApiError carries backend message', msg === 'Member not found.', msg);
  let offline = ''; ctx.SMARTLIB_API_BASE = 'http://127.0.0.1:1';
  console.log(failures === 0 ? '\nALL FRONTEND SMOKE CHECKS PASSED' : '\n' + failures + ' FAILURES');
  process.exit(failures ? 1 : 0);
})().catch((e) => { console.error('CRASH', e); process.exit(2); });
