import {mkdir,copyFile,cp,rm} from 'node:fs/promises';
await rm('dist',{recursive:true,force:true});
await mkdir('dist',{recursive:true});
for(const name of ['index.html','style.css','sha256.js','config.js','auth.js','device.js','app.js','entry.js','overlay-plugin-license.txt','LICENSE.loader','.nojekyll'])await copyFile(name,'dist/'+name);
await cp('loader','dist/loader',{recursive:true});
console.log('Built static GitHub Pages files in dist/');
