"""End-to-end API test: starts the compiled server, drives the real REST API, restarts it to prove persistence.
Usage:  python3 tests/e2e_api.py path/to/smartlibrary
"""
import json, subprocess, time, urllib.request, urllib.error, os, shutil, signal, sys
import tempfile
BINARY=os.path.abspath(sys.argv[1] if len(sys.argv)>1 else 'build/smartlibrary')   # path to the compiled server
TMP=tempfile.mkdtemp(prefix='smartlibrary_e2e_'); DATA=TMP+'/data'; REP=TMP+'/reports'; PORT=8123
def start():
    p=subprocess.Popen([BINARY,'--port',str(PORT),'--data',DATA,'--reports',REP],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,start_new_session=True)
    for _ in range(50):
        try: urllib.request.urlopen(f'http://127.0.0.1:{PORT}/api/health'); return p
        except Exception: time.sleep(0.1)
    raise SystemExit('server did not start')
def stop(p):
    os.killpg(p.pid, signal.SIGTERM); p.wait(); return p.stdout.read()
def call(m,path,body=None):
    req=urllib.request.Request(f'http://127.0.0.1:{PORT}{path}',method=m,data=json.dumps(body).encode() if body is not None else None,headers={'Content-Type':'application/json'} if body is not None else {})
    try:
        r=urllib.request.urlopen(req); return r.status,json.loads(r.read())
    except urllib.error.HTTPError as e: return e.code,json.loads(e.read())
fails=0
def check(name,cond,extra=''):
    global fails
    print(('ok   ' if cond else 'FAIL ')+name+('  '+str(extra) if extra!='' else ''))
    if not cond: fails+=1

p=start()
s,r=call('GET','/api/resources'); check('13 demo resources',len(r['data'])==13)
s,r=call('GET','/api/search?title=harry'); check('search title=harry',r['data']['count']==1)
s,r=call('GET','/api/search?query=ROBERT'); check('search query=ROBERT (case-insensitive, matches author)',r['data']['count']==1 and r['data']['results'][0]['title']=='Clean Code',r['data']['count'])
s,r=call('GET','/api/search?isbn=0164-1212'); check('search ISSN',r['data']['count']==1)
s,r=call('GET','/api/search?genre=fant'); check('search genre partial',r['data']['count']==1)
s,r=call('GET','/api/search?query=%20'); check('empty search -> 400',s==400 and r['message']=='Search query cannot be empty.',r['message'])
# borrow validation
for body,code,msg in [({'memberId':99,'resourceId':1},404,'Member not found.'),({'memberId':6,'resourceId':3},400,'Member account is inactive.'),({'memberId':5,'resourceId':3},400,'Member account is suspended.'),({'memberId':4,'resourceId':1},400,'Resource is currently unavailable.'),({'memberId':1,'resourceId':99},404,'Resource not found.')]:
    s,r=call('POST','/api/loans/borrow',body); check('borrow '+msg,s==code and r['message']==msg,(s,r['message']))
s,r=call('POST','/api/loans/borrow',{'memberId':1,'resourceId':3}); check('student 1 borrows 3rd item',s==201,r['message'])
s,r=call('POST','/api/loans/borrow',{'memberId':1,'resourceId':5}); check('student limit reached',s==400 and r['message']=='Borrowing limit reached.',r['message'])
codes=[call('POST','/api/loans/borrow',{'memberId':7,'resourceId':rid_})[0] for rid_ in (8,10,13)]
s,r=call('GET','/api/members/7'); check('faculty (limit 10) holds more than 3 loans',codes==[201,201,201] and r['data']['activeLoans']==5,(codes,r['data']['activeLoans']))
# reservations
s,r=call('POST','/api/reservations',{'memberId':7,'resourceId':1}); check('reserve unavailable Clean Code -> position 3',s==201 and r['data']['position']==3,r['message'])
s,r=call('POST','/api/reservations',{'memberId':7,'resourceId':1}); check('duplicate reservation -> 409',s==409,r['message'])
s,r=call('POST','/api/reservations',{'memberId':4,'resourceId':8}); check('reserve available resource rejected',s==400 and 'borrow it instead' in r['message'],r['message'])
# return the overdue Clean Code loan of member 1
s,r=call('GET','/api/loans?status=overdue'); overdue=[l for l in r['data'] if l['resourceTitle']=='Clean Code'][0]
check('overdue loan shows 16 days / $8.00 accrued',overdue['overdueDays']==16 and overdue['outstandingFine']==8.0,(overdue['overdueDays'],overdue['outstandingFine']))
s,r=call('GET',f"/api/loans/{overdue['id']}/preview"); check('preview before return: fine 8.00',r['data']['calculatedFine']==8.0,r['data']['finePolicy'])
s,r=call('POST','/api/loans/return',{'loanId':overdue['id'],'memberId':2}); check('return with wrong member rejected',s==400,r['message'])
s,r=call('POST','/api/loans/return',{'loanId':overdue['id'],'memberId':1}); check('return overdue: fine 8.00 + notification',s==200 and r['data']['fine']==8.0 and 'Lê Văn C' in r['data']['notifications'][0],r['data']['notifications'])
s,r=call('POST','/api/loans/return',{'loanId':overdue['id']}); check('second return -> 409',s==409,r['message'])
s,r=call('POST','/api/loans/borrow',{'memberId':8,'resourceId':1}); check('held copy cannot be taken by someone else',s==400 and 'reserved' in r['message'],r['message'])
s,r=call('POST','/api/loans/borrow',{'memberId':3,'resourceId':1}); check('first in queue (Lê Văn C) can borrow held copy',s==201,r['message'])
s,r=call('GET','/api/reservations?memberId=3&resourceId=1'); check('that reservation is COMPLETED',r['data'][0]['status']=='COMPLETED',r['data'][0]['status'])
s,r=call('GET','/api/reservations/queues'); cc=[q for q in r['data'] if q['resource']['title']=='Clean Code'][0]
check('queue now: Phạm Thị D first, then Dr. Huy',[e['memberName'] for e in cc['queue']]==['Phạm Thị D','Dr. Lê Quang Huy'],[e['memberName'] for e in cc['queue']])
s,r=call('POST',f"/api/loans/{overdue['id']}/pay-fine"); check('pay fine',s==200 and r['data']['finePaid'] is True)
s,r=call('POST',f"/api/loans/{overdue['id']}/pay-fine"); check('pay fine twice -> 409',s==409)
# CRUD + validation
good={'type':'BOOK','title':'New Book','author':'Me','isbn':'978-1-86197-876-9','genre':'Test','publicationYear':2021,'totalCopies':2,'publisher':'P','pageCount':100,'edition':'1st'}
s,r=call('POST','/api/resources',good); rid=r['data']['id']; check('create resource',s==201)
s,r=call('POST','/api/resources',good); check('duplicate ISBN -> 409',s==409,r['message'])
s,r=call('POST','/api/resources',dict(good,isbn='9781861978769x',title='X')); check('invalid ISBN -> 400',s==400,r['message'])
s,r=call('POST','/api/resources',dict(good,isbn='978-0-306-40615-7',totalCopies=-3)); check('negative copies -> 400',s==400,r['message'])
s,r=call('PUT',f'/api/resources/{rid}',{'title':'Renamed','totalCopies':5}); check('update resource',s==200 and r['data']['title']=='Renamed' and r['data']['totalCopies']==5)
s,r=call('DELETE',f'/api/resources/{rid}'); check('delete resource',s==200)
s,r=call('DELETE','/api/resources/1'); check('delete resource with open reservations/loans refused',s==409,r['message'])
s,r=call('POST','/api/members',{'type':'STUDENT','fullName':'X','email':'bad'}); check('invalid email -> 400',s==400,r['message'])
s,r=call('POST','/api/members',{'type':'FACULTY','fullName':'Dr. Test','email':'dr.test@faculty.example.edu','phone':'+84 1'}); mid=r['data']['id']; check('create faculty member',s==201 and r['data']['borrowLimit']==10)
s,r=call('PUT',f'/api/members/{mid}',{'status':'SUSPENDED'}); s2,r2=call('POST','/api/loans/borrow',{'memberId':mid,'resourceId':8}); check('suspended member cannot borrow',s2==400)
# reports
s,r=call('GET','/api/reports/inventory'); txt=open(REP+'/inventory_report.txt').read()
check('inventory report file written',s==200 and 'SMART LIBRARY INVENTORY REPORT' in txt and 'DEMO' in txt)
s,r=call('GET','/api/reports/csv'); check('csv report written',os.path.exists(REP+'/inventory_report.csv'))
print('\n----- report -----\n'+txt)
# persistence across restart
s,before_l=call('GET','/api/loans'); s,before_r=call('GET','/api/reservations'); s,before_res=call('GET','/api/resources'); s,before_d=call('GET','/api/dashboard')
log=stop(p); p=start()
s,after_l=call('GET','/api/loans'); s,after_r=call('GET','/api/reservations'); s,after_res=call('GET','/api/resources'); s,after_d=call('GET','/api/dashboard')
check('loans identical after restart',before_l['data']==after_l['data'],len(after_l['data']))
check('reservations identical after restart',before_r['data']==after_r['data'])
check('resources identical after restart',before_res['data']==after_res['data'])
check('dashboard stats identical after restart',before_d['data']['stats']==after_d['data']['stats'])
s,r=call('POST','/api/loans/borrow',{'memberId':4,'resourceId':13}); check('new loan id continues sequence',r['data']['id']==len(after_l['data'])+1,r['data']['id'])
# reset requires confirmation
s,r=call('POST','/api/data/reset-demo',{}); check('reset without confirm refused',s==400)
s,r=call('POST','/api/data/reset-demo',{'confirm':True}); s,h=call('GET','/api/health'); check('reset with confirm restores demo',h['data']['resources']==13 and h['data']['demoData'])
# robustness
req=urllib.request.Request(f'http://127.0.0.1:{PORT}/api/loans/borrow',method='POST',data=b'{bad json',headers={'Content-Type':'application/json'})
try: urllib.request.urlopen(req)
except urllib.error.HTTPError as e: check('malformed JSON -> 400 (no crash)',e.code==400)
s,h=call('GET','/api/health'); check('server still alive',s==200)
out=stop(p); print('\nserver log tail:\n'+'\n'.join(out.strip().splitlines()[-4:]))
shutil.rmtree(TMP,ignore_errors=True)
print('\nE2E RESULT:', 'ALL PASSED' if fails==0 else f'{fails} FAILED'); sys.exit(1 if fails else 0)
