import assert from 'node:assert/strict';
import {readFileSync,readdirSync,existsSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import vm from 'node:vm';
const root=new URL('../',import.meta.url);
// Build the original user-uploaded FTP bytes from their exact git-tracked archive.
execFileSync('python3',['scripts/materialize-user-ftp.py'],{cwd:new URL('.',root),stdio:'pipe'});
const read=p=>readFileSync(new URL(p,root),'utf8');
const bytes=p=>new Uint8Array(readFileSync(new URL(p,root)));
const ps4=fw=>`Mozilla/5.0 (PlayStation 4 ${fw}) AppleWebKit/605.1.15`;
const testPassword='local-test-password';
const testHash=createHash('sha256').update(testPassword).digest('hex');
const lifetime=8*60*60*1000;
const promoted=existsSync(new URL('loader/dexcore.bin',root));
const expectedDefaultPayload=promoted?'loader/'+read('payloads.js').match(/\["(dexcore\.bin\?v=[^"]+)",500448/)[1]:'loader/henacy.bin?v=1-syslogo-ea1235b846b0';

function environment(ua,{search='',session=new Map(),preference=new Map(),corrupt=false,storageBlocked=false}={}){
  const nodes=new Map(),intervals=new Map(),navigation=[],requests=[],pending=[],modules=[];let now=0,next=0;
  function node(id){if(!nodes.has(id))nodes.set(id,{textContent:'',value:'',hidden:id==='hostPanel',disabled:id==='loadExploit'||id==='toggleAuto',handlers:{},focus(){},addEventListener(type,fn){this.handlers[type]=fn;},click(){if(!this.disabled)this.handlers.click?.();},submit(){this.handlers.submit?.({preventDefault(){}});}});return nodes.get(id);}
  const storage=map=>({getItem(k){if(storageBlocked)throw Error('storage');return map.get(k)||null;},setItem(k,v){if(storageBlocked)throw Error('storage');map.set(k,v);},removeItem(k){map.delete(k);}});
  class FakeDate extends Date{static now(){return now;}}
  class XHR{
    open(_method,path){this.path=path;requests.push(path);}
    send(){pending.push(Promise.resolve().then(()=>{this.status=200;const data=bytes(this.path);if(corrupt&&this.path.split('?')[0].endsWith('.bin')&&!this.path.includes('patches/'))data[data.length-1]^=1;this.response=data.buffer;this.onload();}));}
  }
  const context=vm.createContext({HEN_CONFIG:{passwordHash:testHash},navigator:{userAgent:ua},document:{getElementById:node,createElement:()=>({}),head:{appendChild:element=>modules.push(element)}},sessionStorage:storage(session),localStorage:storage(preference),location:{search,assign:path=>navigation.push(path),replace:path=>navigation.push(path)},XMLHttpRequest:XHR,Date:FakeDate,Uint8Array,ArrayBuffer,DataView,Int32Array,Promise,unescape,encodeURIComponent,setInterval:fn=>{intervals.set(++next,fn);return next;},clearInterval:id=>intervals.delete(id)});
  context.window=context;
  for(const script of ['sha256.js','auth.js','device.js','payloads.js','ftp-payload.js','app.js','entry.js'])vm.runInContext(read(script),context,{filename:script});
  return {context,node,intervals,navigation,requests,modules,session,login(password=testPassword){node('password').value=password;node('loginForm').submit();},async settle(){await Promise.all(pending);await new Promise(resolve=>setImmediate(resolve));},async advance(ms){now+=ms;for(const fn of [...intervals.values()])fn();await this.settle();},loadEntry(){vm.runInContext(read('loader/entry.js'),context);}};
}

const gated=environment(ps4('13.52'));
assert.equal(gated.node('hostPanel').hidden,true);assert.equal(gated.intervals.size,0);
await gated.advance(30000);assert.equal(gated.requests.length,0);
gated.login('wrong');assert.equal(gated.node('hostPanel').hidden,true);assert.match(gated.node('loginError').textContent,/Incorrect password/);assert.equal(gated.node('password').value,'');
gated.login();assert.equal(gated.node('hostPanel').hidden,false);assert.equal(gated.node('loginPanel').hidden,true);assert.equal(gated.node('loadExploit').disabled,false);assert.match(gated.node('status').textContent,/10 seconds/);
await gated.advance(9999);assert.equal(gated.requests.length,0);await gated.advance(1);assert.deepEqual(gated.navigation,['loader/jb.html?v=dexcore-ui-1-1'],JSON.stringify({requests:gated.requests,status:gated.node('status').textContent}));
console.log('PASS: no countdown or launch before password entry; wrong password gives feedback; correct password starts ten seconds.');

const session=new Map(),preference=new Map(),disabled=environment(ps4('13.52'),{session,preference});
disabled.login();disabled.node('toggleAuto').click();await disabled.advance(30000);assert.equal(disabled.requests.length,0);
const remembered=environment(ps4('13.52'),{session,preference});assert.equal(remembered.node('hostPanel').hidden,false);assert.equal(remembered.intervals.size,0);assert.equal(remembered.node('toggleAuto').textContent,'Enable auto loading');
remembered.node('loadExploit').click();await remembered.settle();assert.deepEqual(remembered.navigation,['loader/jb.html?v=dexcore-ui-1-1']);
remembered.node('logout').click();assert.equal(remembered.context.HEN_AUTH.allowed(),false);assert.equal(remembered.navigation.at(-1),'./');
const blockedStorage=environment(ps4('13.52'),{storageBlocked:true});blockedStorage.login();assert.equal(blockedStorage.node('hostPanel').hidden,true);assert.match(blockedStorage.node('loginError').textContent,/storage/);
console.log('PASS: remembered auto-load preference, manual launch, session restoration, logout, and storage failure feedback.');


for(const ua of ['Desktop browser',ps4('13.50'),ps4('14.00'),ps4('13.52.1'),ps4('13.520'),'Mozilla/5.0 (PlayStation 5 13.52)']){
  for(const search of ['', '?original=1', '?legacy=1']){const unsupported=environment(ua,{search});unsupported.login();assert.equal(unsupported.node('loadExploit').disabled,true);assert.equal(unsupported.node('toggleAuto').disabled,true);await unsupported.advance(60000);assert.equal(unsupported.requests.length,0);unsupported.loadEntry();assert.equal(unsupported.modules.length,0);}
}
for(const search of ['', '?original=1', '?legacy=1']){const corrupt=environment(ps4('13.52'),{search,corrupt:true});corrupt.login();corrupt.node('loadExploit').click();await corrupt.settle();assert.equal(corrupt.navigation.length,0);assert.match(corrupt.node('status').textContent,/verification failed/);assert.equal(corrupt.intervals.size,0);}
const current=environment(ps4('13.52'));current.login();
assert.equal(current.node('hostTitle').textContent,'DEXCORE');
assert.equal(current.context.HEN_BUILD.version,'1.1');
assert.equal(current.node('diagnostics').href,'loader/diagnostics.html');
await current.advance(10000);
assert.deepEqual(current.navigation,['loader/jb.html?v=dexcore-ui-1-1']);
assert.equal(current.requests[0],expectedDefaultPayload);
assert.equal(gated.requests[0],expectedDefaultPayload);
if(promoted){
  assert.equal(gated.context.HEN_BUILD.payload[2],createHash('sha256').update(bytes('loader/dexcore.bin')).digest('hex'));
  const dexcore=Buffer.from(bytes('loader/dexcore.bin'));
  assert.equal(dexcore.length,500448);
  assert(dexcore.includes(Buffer.from('DEXCORE')));
  if(dexcore.includes(Buffer.from('User FTPS4 / TCP 1337'))){
    assert.deepEqual([...dexcore.subarray(0x50155,0x5015a)],[144,144,144,144,144]);
    assert.deepEqual([...dexcore.subarray(0x5013e,0x50143)],[232,77,24,0,0]);
  }
  assert(dexcore.includes(Buffer.from('FTP / Payload Options')) || dexcore.includes(Buffer.from('FTP Server')));
  if(dexcore.includes(Buffer.from('ftpsrv a717ffd'))){
    assert(dexcore.includes(Buffer.from('DEXCore already loaded. Skipping...')));
    assert(dexcore.includes(Buffer.from('FTP Server')));
    assert(!dexcore.includes(Buffer.from('HNCY restarting')));
    assert(!dexcore.includes(Buffer.from('HENACY already installed')));
  }
  assert(dexcore.includes(Buffer.from(' | DEXCore\0')) || dexcore.includes(Buffer.from(' | DEXCORE\0')));
  if (dexcore.includes(Buffer.from('title="DEXCore v1" file="hen_settings.xml"'))) {
    assert(!dexcore.includes(Buffer.from('title="★ HNCY" file="hen_settings.xml"')));
  }
  if (dexcore.includes(Buffer.from('title="DEXCORE" file="hen_settings.xml"'))) {
    assert(dexcore.includes(Buffer.from('File System')));
    assert(dexcore.includes(Buffer.from('System Settings')));
    assert(dexcore.includes(Buffer.from(' | DEXCORE\0')));
    assert(dexcore.includes(Buffer.from(bytes('assets/hen_settings_icon.png'))));
  }
  assert.equal(createHash('sha256').update(dexcore).digest('hex'),gated.context.HEN_BUILD.payload[2]);
}

assert.equal(gated.node('hostTitle').textContent,'DEXCORE');
assert.equal(gated.context.document.title,'DEXCORE // 1.1');
if(promoted) assert.equal(gated.context.HEN_BUILD.payload[0].split('?')[0],'dexcore.bin');
for(const env of [gated,current]){
  const declaration=read('loader/jb.js').match(/const PAYLOAD_FILE = ([^;]+);/)[1];
  assert.equal(vm.runInContext('const FTP_MODE=false; const payloadSpec=null; '+declaration,env.context),env.context.HEN_BUILD.payload[0]);
}
const guestLegacy=environment(ps4('13.52'),{search:'?legacy=1'});guestLegacy.loadEntry();
assert.deepEqual(guestLegacy.navigation,['../index.html']);
assert.equal(guestLegacy.context.document.title,'DEXCORE // 1.1');
for(const search of ['?original=1','?legacy=1','?henacy=1','?vsh=1','?original=1&legacy=1','?vsh=1&original=1']){
  const c=vm.createContext({location:{search}});c.window=c;
  vm.runInContext(read('payloads.js'),c);
  assert.equal(c.HEN_BUILD.label,'DEXCORE');
  assert.equal(c.HEN_BUILD.original,false);
  assert.equal(c.HEN_BUILD.query,'');
  assert.equal(c.HEN_BUILD.payload[0],gated.context.HEN_BUILD.payload[0]);
}
const singleVersionHTML=read('index.html');
assert(!/id="switchBuild"|href="\\?(?:original|legacy)=1"/.test(singleVersionHTML));
assert(!/hen\\.bin|henacy\\.bin/.test(read('payloads.js')));
console.log('PASS: exactly one DEXCore build is selectable; legacy query parameters cannot load old HEN payloads; PS4 13.52 gate and integrity checks retained.');

const providedFtp=bytes('loader/user-ftp.bin');
const ftpDigest=createHash('sha256').update(providedFtp).digest('hex');
assert.equal(providedFtp.length,29052);
assert.equal(ftpDigest,'b4b984524ac63508255175c76bdf942ebfbd577234337bfd91e2712e27738560');
assert.equal(gated.context.DEXCORE_FTP.sha256,ftpDigest);
assert.equal(gated.context.DEXCORE_FTP.port,1337);
assert.equal(providedFtp[0],0xe9);
const ftpHost=environment(ps4('13.52'));ftpHost.login();
ftpHost.node('loadUserFtp').click();await ftpHost.settle();
assert.deepEqual(ftpHost.navigation,['loader/jb.html?ftp=1&v=dexcore-userftp-1']);
assert.equal(ftpHost.requests[0],'loader/user-ftp.bin');
assert.equal(ftpHost.context.HEN_SHA256(providedFtp),ftpDigest);
const ftpBad=environment(ps4('13.52'),{corrupt:true});ftpBad.login();
ftpBad.node('loadUserFtp').click();await ftpBad.settle();
assert.equal(ftpBad.navigation.length,0);
assert.match(ftpBad.node('status').textContent,/verification failed/);
const desktopFtp=environment('Desktop browser');desktopFtp.login();
assert.equal(desktopFtp.node('loadUserFtp').disabled,true);
assert.match(read('loader/jb.js'),/payloadVerified\(payloadBlob\)/);
assert.match(read('loader/jb.js'),/payloadVerified\(blob\)/);
assert.match(read('loader/jb.js'),/params\.get\("ftp"\) === "1"/);
console.log('PASS: exact user-uploaded FTPS4 binary restored, SHA-verified, 1337 launch route, corrupted payload rejection and direct-loader revalidation.');


const guestLoader=environment(ps4('13.52'));guestLoader.loadEntry();assert.deepEqual(guestLoader.navigation,['../index.html']);assert.equal(guestLoader.modules.length,0);
const allowedLoader=environment(ps4('13.52'));allowedLoader.login();allowedLoader.loadEntry();assert.equal(allowedLoader.modules.length,1);assert.match(allowedLoader.modules[0].src,/^jb.js\?/);
await allowedLoader.advance(lifetime+1);assert.equal(allowedLoader.context.HEN_AUTH.allowed(),false);
console.log('PASS: direct loader requires the browser gate and matching firmware before it imports the exploit module; sessions expire.');

const hashes={
 'loader/henacy.bin':['ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352',500448],
 'loader/hen.bin':['568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e',499776],
 'loader/patches/1352.bin':['adfb9771904f71cd1f5a82cbded8ac46c7ffeb3df974579535cf711904ac347e',632]
};
for(const [path,[hash,size]] of Object.entries(hashes)){const data=bytes(path);assert.equal(data.length,size);assert.equal(createHash('sha256').update(data).digest('hex'),hash);assert.equal(allowedLoader.context.HEN_SHA256(data),hash);}
const menuIcon=Buffer.from(bytes('assets/hen_settings_icon.png'));
const brandedPayload=Buffer.from(bytes('loader/henacy.bin'));
const pngMagic=Buffer.from([137,80,78,71,13,10,26,10]);
assert(menuIcon.subarray(0,8).equals(pngMagic));
assert.equal(menuIcon.readUInt32BE(16),128);assert.equal(menuIcon.readUInt32BE(20),128);
assert.equal(menuIcon[24],8);assert.equal(menuIcon[25],6);
assert.equal(createHash('sha256').update(menuIcon).digest('hex'),'6fb896597b4dbcfdf88595fe12c9d2f14627f455222417f3aceea95493aa2712');
const iconOffset=brandedPayload.indexOf(pngMagic);
assert(iconOffset>=0,'The menu image must be embedded in the payload.');
assert.equal(brandedPayload.indexOf(pngMagic),iconOffset);
assert.equal(brandedPayload.indexOf(pngMagic,iconOffset+1),-1);
assert.equal(createHash('sha256').update(brandedPayload.subarray(iconOffset,iconOffset+menuIcon.length)).digest('hex'),'a81ec5f1131b6a4595674b2239429669e10bf3a3fb69ed77e389490c933378ac');
const selfMagic=Buffer.from([79,21,61,29]),elfMagic=Buffer.from([127,69,76,70]);
let iconWriters=0;
for(let base=brandedPayload.indexOf(selfMagic);base>=0;base=brandedPayload.indexOf(selfMagic,base+4)){
  if(base+32>brandedPayload.length)continue;
  const count=brandedPayload.readUInt16LE(base+24),elf=base+32+count*32;
  if(count>64||elf+64>brandedPayload.length||!brandedPayload.subarray(elf,elf+4).equals(elfMagic))continue;
  const phoff=Number(brandedPayload.readBigUInt64LE(elf+32)),stride=brandedPayload.readUInt16LE(elf+54),segments=[];
  for(let i=0;i<count;i++){
    const entry=base+32+i*32,flags=brandedPayload.readBigUInt64LE(entry);
    if(!(flags&0x800n))continue;
    const ph=elf+phoff+Number((flags>>20n)&0xfffn)*stride,type=brandedPayload.readUInt32LE(ph);
    if(type!==1&&type!==0x61000010)continue;
    segments.push({file:base+Number(brandedPayload.readBigUInt64LE(entry+8)),size:Number(brandedPayload.readBigUInt64LE(entry+16)),virtual:Number(brandedPayload.readBigUInt64LE(ph+16)),flags:brandedPayload.readUInt32LE(ph+4)});
  }
  if(!segments.some(s=>s.file<=iconOffset&&iconOffset+menuIcon.length<=s.file+s.size))continue;
  const map=(address,length=1)=>{const s=segments.find(s=>s.virtual<=address&&address+length<=s.virtual+s.size);return s?s.file+address-s.virtual:undefined;};
  for(const executable of segments.filter(s=>s.flags&1))for(let at=executable.file+7;at+17<=executable.file+executable.size;at++){
    if(!brandedPayload.subarray(at,at+3).equals(Buffer.from([72,141,53]))||brandedPayload[at+7]!==186||brandedPayload[at+12]!==232)continue;
    if(!brandedPayload.subarray(at-7,at-4).equals(Buffer.from([72,141,61])))continue;
    const pathAddress=executable.virtual+at-executable.file+brandedPayload.readInt32LE(at-4),pathOffset=map(pathAddress);
    if(pathOffset===undefined)continue;
    const pathEnd=brandedPayload.indexOf(0,pathOffset);
    if(brandedPayload.subarray(pathOffset,pathEnd).toString()!=='/data/hen/shellui_data/icons/hen_settings_icon.png')continue;
    const imageAddress=executable.virtual+at-executable.file+7+brandedPayload.readInt32LE(at+3),length=brandedPayload.readUInt32LE(at+8);
    assert.equal(length,menuIcon.length);assert.equal(map(imageAddress,length),iconOffset);iconWriters++;
  }
}
assert.equal(iconWriters,1,'The menu plugin must write the new image using its exact pointer and length.');
console.log('PASS: native icon writer and 16,364-byte resource location retained; new DEXCORE image is guarded in native build.');
const menuSelf=brandedPayload.indexOf(selfMagic,0x29c80);
assert.equal(menuSelf,0x29c80);
const menuElf=menuSelf+32+brandedPayload.readUInt16LE(menuSelf+24)*32;
const menuPh=menuElf+Number(brandedPayload.readBigUInt64LE(menuElf+32));
const menuStride=brandedPayload.readUInt16LE(menuElf+54);
assert.equal(Number(brandedPayload.readBigUInt64LE(menuPh+menuStride*2+40)),0x2c00);
const menuText=menuSelf+Number(brandedPayload.readBigUInt64LE(menuSelf+32+32+8));
assert.equal(brandedPayload[menuText+0x1d0f],0xe8);
const infoEntry=0x1d14+brandedPayload.readInt32LE(menuText+0x1d10);
assert(infoEntry>=0x12040&&infoEntry<0x13f80,'Boot must invoke the mapped System Information extension.');
assert(brandedPayload.includes(Buffer.from('SystemSoftwareVersionInfo\0')));
assert(brandedPayload.includes(Buffer.from('DisplayVersion\0')));
assert(brandedPayload.includes(Buffer.from('hncy_system_information_logo\0')));
assert(brandedPayload.indexOf(Buffer.from('file:///data/hen/shellui_data/icons/hen_settings_icon.png\0'),0x39000)>0x39000, 'System Information must use the same image URI as the HNCY menu.');
console.log('PASS: System Information branding is connected to plugin boot and references the native version field and HNCY image.');
for(const page of ['index.html','loader/jb.html'])for(const match of read(page).matchAll(/(?:src|href)="([^"#]+)"/g)){
  const url=match[1];if(url==='data:,'||/^https?:\/\//.test(url))continue;assert(!url.startsWith('/'),'absolute path breaks a project Pages site: '+url);assert(existsSync(new URL(url,new URL(page,root))),url);
}
assert.match(read('loader/diagnostics.html'),/It does not start the exploit or modify the console/);
assert(existsSync(new URL('.nojekyll',root)));assert(existsSync(new URL('README.md',root)),'README should exist');assert.doesNotMatch(read('config.js'),/local-test-password/);
console.log('PASS: internal source/reference payload hashes match; 13.52 patch preserved; all paths work under a project subdirectory.');

const retiredBrand=['PRO','HEN'].join('');
for(const path of ['README.md','index.html','loader/jb.html','loader/diagnostics.html','entry.js','payloads.js'])
  assert(!read(path).toUpperCase().includes(retiredBrand), path+' contains obsolete branding');
assert.match(read('README.md'),/^# DEXCORE 1.1/m);
assert.match(read('index.html'),/<title>DEXCORE \/\/ 1.1<\/title>/);
assert.match(read('loader/jb.html'),/<title>DEXCORE 1.1/);
console.log('PASS: DEXCORE 1.1 branding applied to site and loader without changing original HEN binary.');
