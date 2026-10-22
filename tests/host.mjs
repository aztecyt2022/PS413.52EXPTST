import assert from 'node:assert/strict';
import {readFileSync,readdirSync,existsSync} from 'node:fs';
import {createHash} from 'node:crypto';
import vm from 'node:vm';
const root=new URL('../',import.meta.url);
const read=p=>readFileSync(new URL(p,root),'utf8');
const bytes=p=>new Uint8Array(readFileSync(new URL(p,root)));
const ps4=fw=>`Mozilla/5.0 (PlayStation 4 ${fw}) AppleWebKit/605.1.15`;
const testPassword='local-test-password';
const testHash=createHash('sha256').update(testPassword).digest('hex');
const lifetime=8*60*60*1000;

function environment(ua,{overlay=false,original=false,session=new Map(),preference=new Map(),corrupt=false,storageBlocked=false}={}){
  const nodes=new Map(),intervals=new Map(),navigation=[],requests=[],pending=[],modules=[];let now=0,next=0;
  function node(id){if(!nodes.has(id))nodes.set(id,{textContent:'',value:'',hidden:id==='hostPanel',disabled:id==='loadExploit'||id==='toggleAuto',handlers:{},focus(){},addEventListener(type,fn){this.handlers[type]=fn;},click(){if(!this.disabled)this.handlers.click?.();},submit(){this.handlers.submit?.({preventDefault(){}});}});return nodes.get(id);}
  const storage=map=>({getItem(k){if(storageBlocked)throw Error('storage');return map.get(k)||null;},setItem(k,v){if(storageBlocked)throw Error('storage');map.set(k,v);},removeItem(k){map.delete(k);}});
  class FakeDate extends Date{static now(){return now;}}
  class XHR{
    open(_method,path){this.path=path;requests.push(path);}
    send(){pending.push(Promise.resolve().then(()=>{this.status=200;const data=bytes(this.path);if(corrupt&&this.path.split('?')[0].endsWith('.bin')&&!this.path.includes('patches/'))data[data.length-1]^=1;this.response=data.buffer;this.onload();}));}
  }
  const context=vm.createContext({HEN_CONFIG:{passwordHash:testHash},navigator:{userAgent:ua},document:{getElementById:node,createElement:()=>({}),head:{appendChild:element=>modules.push(element)}},sessionStorage:storage(session),localStorage:storage(preference),location:{search:original?'?original=1':overlay?'?overlay=1':'',assign:path=>navigation.push(path),replace:path=>navigation.push(path)},XMLHttpRequest:XHR,Date:FakeDate,Uint8Array,ArrayBuffer,DataView,Int32Array,Promise,unescape,encodeURIComponent,setInterval:fn=>{intervals.set(++next,fn);return next;},clearInterval:id=>intervals.delete(id)});
  context.window=context;
  for(const script of ['sha256.js','auth.js','device.js','payloads.js','app.js','entry.js'])vm.runInContext(read(script),context,{filename:script});
  return {context,node,intervals,navigation,requests,modules,session,login(password=testPassword){node('password').value=password;node('loginForm').submit();},async settle(){await Promise.all(pending);await new Promise(resolve=>setImmediate(resolve));},async advance(ms){now+=ms;for(const fn of [...intervals.values()])fn();await this.settle();},loadEntry(){vm.runInContext(read('loader/entry.js'),context);}};
}

const gated=environment(ps4('13.52'));
assert.equal(gated.node('hostPanel').hidden,true);assert.equal(gated.intervals.size,0);
await gated.advance(30000);assert.equal(gated.requests.length,0);
gated.login('wrong');assert.equal(gated.node('hostPanel').hidden,true);assert.match(gated.node('loginError').textContent,/Incorrect password/);assert.equal(gated.node('password').value,'');
gated.login();assert.equal(gated.node('hostPanel').hidden,false);assert.equal(gated.node('loginPanel').hidden,true);assert.equal(gated.node('loadExploit').disabled,false);assert.match(gated.node('status').textContent,/10 seconds/);
await gated.advance(9999);assert.equal(gated.requests.length,0);await gated.advance(1);assert.deepEqual(gated.navigation,['loader/jb.html'],JSON.stringify({requests:gated.requests,status:gated.node('status').textContent}));
console.log('PASS: no countdown or launch before password entry; wrong password gives feedback; correct password starts ten seconds.');

const session=new Map(),preference=new Map(),disabled=environment(ps4('13.52'),{session,preference});
disabled.login();disabled.node('toggleAuto').click();await disabled.advance(30000);assert.equal(disabled.requests.length,0);
const remembered=environment(ps4('13.52'),{session,preference});assert.equal(remembered.node('hostPanel').hidden,false);assert.equal(remembered.intervals.size,0);assert.equal(remembered.node('toggleAuto').textContent,'Enable auto loading');
remembered.node('loadExploit').click();await remembered.settle();assert.deepEqual(remembered.navigation,['loader/jb.html']);
remembered.node('logout').click();assert.equal(remembered.context.HEN_AUTH.allowed(),false);assert.equal(remembered.navigation.at(-1),'./');
const blockedStorage=environment(ps4('13.52'),{storageBlocked:true});blockedStorage.login();assert.equal(blockedStorage.node('hostPanel').hidden,true);assert.match(blockedStorage.node('loginError').textContent,/storage/);
console.log('PASS: remembered auto-load preference, manual launch, session restoration, logout, and storage failure feedback.');

for(const ua of ['Desktop browser',ps4('13.50'),ps4('14.00'),ps4('13.52.1'),ps4('13.520'),'Mozilla/5.0 (PlayStation 5 13.52)']){
  const unsupported=environment(ua);unsupported.login();assert.equal(unsupported.node('loadExploit').disabled,true);assert.equal(unsupported.node('toggleAuto').disabled,true);await unsupported.advance(60000);assert.equal(unsupported.requests.length,0);unsupported.loadEntry();assert.equal(unsupported.modules.length,0);
}
const overlay=environment(ps4('13.52'),{overlay:true});overlay.login();assert.equal(overlay.intervals.size,0);assert.equal(overlay.node('toggleAuto').disabled,true);await overlay.advance(60000);assert.equal(overlay.requests.length,0);overlay.node('loadExploit').click();await overlay.settle();assert.deepEqual(overlay.navigation,['loader/jb.html?overlay=1']);assert.equal(overlay.requests[0],'loader/prohen.bin?v=2.2.8-build4');
for(const option of [{},{overlay:true},{original:true}]){const corrupt=environment(ps4('13.52'),{...option,corrupt:true});corrupt.login();corrupt.node('loadExploit').click();await corrupt.settle();assert.equal(corrupt.navigation.length,0);assert.match(corrupt.node('status').textContent,/verification failed/);assert.equal(corrupt.intervals.size,0);}
const original=environment(ps4('13.52'),{original:true});original.login();
assert.equal(original.node('hostTitle').textContent,'PS4 HEN 2.2.0');
assert.equal(original.node('overlayNote').hidden,true);
original.node('loadExploit').click();await original.settle();
assert.deepEqual(original.navigation,['loader/jb.html?original=1']);
assert.equal(original.requests[0],'loader/hen.bin');
assert.equal(gated.requests[0],'loader/prohen.bin?v=2.2.8-build4');
assert.equal(gated.node('hostTitle').textContent,'PROHEN 2.2.8');
assert.equal(gated.node('overlayNote').hidden,false);
for(const env of [gated,overlay,original]){
  const declaration=read('loader/jb.js').match(/const PAYLOAD_FILE = ([^;]+);/)[1];
  assert.equal(vm.runInContext(declaration,env.context),env.context.HEN_BUILD.payload[0]);
}
const guestOriginal=environment(ps4('13.52'),{original:true});guestOriginal.loadEntry();
assert.deepEqual(guestOriginal.navigation,['../index.html?original=1']);
console.log('PASS: exact PS4 13.52 guard; PROHEN is the default; original HEN fallback and manual mode select the matching loader payload; corrupt payloads stop all launch paths.');

const guestLoader=environment(ps4('13.52'),{overlay:true});guestLoader.loadEntry();assert.deepEqual(guestLoader.navigation,['../index.html?overlay=1']);assert.equal(guestLoader.modules.length,0);
const allowedLoader=environment(ps4('13.52'));allowedLoader.login();allowedLoader.loadEntry();assert.equal(allowedLoader.modules.length,1);assert.match(allowedLoader.modules[0].src,/^jb.js\?/);
await allowedLoader.advance(lifetime+1);assert.equal(allowedLoader.context.HEN_AUTH.allowed(),false);
console.log('PASS: direct loader requires the browser gate and matching firmware before it imports the exploit module; sessions expire.');

const hashes={
 'loader/hen.bin':['568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e',499776],
 'loader/prohen.bin':['f04082626ee6bae2484e72d9a392f24ab5bcfe007e3793a464ff4ab058b54f4f',587904],
 'loader/patches/1352.bin':['adfb9771904f71cd1f5a82cbded8ac46c7ffeb3df974579535cf711904ac347e',632]
};
for(const [path,[hash,size]] of Object.entries(hashes)){const data=bytes(path);assert.equal(data.length,size);assert.equal(createHash('sha256').update(data).digest('hex'),hash);assert.equal(allowedLoader.context.HEN_SHA256(data),hash);}
for(const page of ['index.html','loader/jb.html'])for(const match of read(page).matchAll(/(?:src|href)="([^"#]+)"/g)){
  const url=match[1];if(url==='data:,')continue;assert(!url.startsWith('/'),'absolute path breaks a project Pages site: '+url);assert(existsSync(new URL(url,new URL(page,root))),url);
}
assert(existsSync(new URL('.nojekyll',root)));assert.match(read('README.md'),/not secure access/);assert.doesNotMatch(read('config.js'),/local-test-password/);
console.log('PASS: PROHEN and original payload hashes match; patch bytes preserved; independent checksums match; HTML asset paths work under a project subdirectory.');
