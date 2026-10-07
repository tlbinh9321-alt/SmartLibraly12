/* API client. The frontend never applies business rules: it sends requests to the
   C++ backend and renders whatever {success, message, data} comes back. */
(function (root) {
  class ApiError extends Error {
    constructor(message, status) { super(message); this.name = 'ApiError'; this.status = status; }
  }

  const BASE = root.SMARTLIB_API_BASE || '';

  async function request(method, path, body) {
    let response;
    try {
      response = await fetch(BASE + path, {
        method,
        headers: body !== undefined ? { 'Content-Type': 'application/json' } : {},
        body: body !== undefined ? JSON.stringify(body) : undefined,
      });
    } catch (e) {
      throw new ApiError('Cannot reach the C++ backend. Is the server running?', 0);
    }
    let json;
    try { json = await response.json(); }
    catch (e) { throw new ApiError('Unexpected response from server (HTTP ' + response.status + ').', response.status); }
    if (!json.success) throw new ApiError(json.message || 'Request failed.', response.status);
    return json;  // { success: true, message, data }
  }

  function qs(params) {
    const parts = [];
    Object.keys(params || {}).forEach(function (k) {
      const v = params[k];
      if (v !== undefined && v !== null && String(v).trim() !== '') parts.push(encodeURIComponent(k) + '=' + encodeURIComponent(String(v).trim()));
    });
    return parts.length ? '?' + parts.join('&') : '';
  }

  const API = {
    ApiError,
    health: () => request('GET', '/api/health'),
    dashboard: () => request('GET', '/api/dashboard'),
    // resources
    resources: (filters) => request('GET', '/api/resources' + qs(filters)),
    resource: (id) => request('GET', '/api/resources/' + id),
    createResource: (body) => request('POST', '/api/resources', body),
    updateResource: (id, body) => request('PUT', '/api/resources/' + id, body),
    deleteResource: (id) => request('DELETE', '/api/resources/' + id),
    genres: () => request('GET', '/api/genres'),
    // members
    members: (filters) => request('GET', '/api/members' + qs(filters)),
    member: (id) => request('GET', '/api/members/' + id),
    createMember: (body) => request('POST', '/api/members', body),
    updateMember: (id, body) => request('PUT', '/api/members/' + id, body),
    // loans
    loans: (filters) => request('GET', '/api/loans' + qs(filters)),
    previewReturn: (id) => request('GET', '/api/loans/' + id + '/preview'),
    borrow: (memberId, resourceId) => request('POST', '/api/loans/borrow', { memberId: memberId, resourceId: resourceId }),
    returnLoan: (loanId) => request('POST', '/api/loans/return', { loanId: loanId }),
    payFine: (loanId) => request('POST', '/api/loans/' + loanId + '/pay-fine', {}),
    // reservations
    reservations: (filters) => request('GET', '/api/reservations' + qs(filters)),
    queues: () => request('GET', '/api/reservations/queues'),
    reserve: (memberId, resourceId) => request('POST', '/api/reservations', { memberId: memberId, resourceId: resourceId }),
    cancelReservation: (id) => request('DELETE', '/api/reservations/' + id),
    // search
    search: (params) => request('GET', '/api/search' + qs(params)),
    // reports + system
    analytics: () => request('GET', '/api/reports/analytics'),
    inventoryReport: () => request('GET', '/api/reports/inventory'),
    inventoryCsv: () => request('GET', '/api/reports/csv'),
    saveData: () => request('POST', '/api/data/save', {}),
    loadData: () => request('POST', '/api/data/load', {}),
    resetDemo: () => request('POST', '/api/data/reset-demo', { confirm: true }),
    settings: () => request('GET', '/api/settings'),
    saveSettings: (body) => request('PUT', '/api/settings', body),
  };
  root.API = API;
})(typeof window !== 'undefined' ? window : globalThis);
