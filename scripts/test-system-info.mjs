import {mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../',import.meta.url));
const folder=mkdtempSync(join(tmpdir(),'henacy-system-info-'));
try{
  const binary=join(folder,'native-test');
  let result=spawnSync(process.env.CC||'cc',['-Os','-Wall','-Wextra','-Werror','tests/system-info.c','-o',binary],{cwd:root,stdio:'inherit'});
  if(result.error)throw result.error;
  if(result.status!==0)throw Error('Could not compile the native System Information test.');
  result=spawnSync(binary,process.env.DEXCORE_TEST_PAYLOAD?[process.env.DEXCORE_TEST_PAYLOAD]:[],{cwd:root,stdio:'inherit'});
  if(result.error)throw result.error;
  if(result.status!==0)throw Error('Native System Information test failed.');
}finally{rmSync(folder,{recursive:true,force:true});}
