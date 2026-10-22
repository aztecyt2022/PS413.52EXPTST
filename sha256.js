(function (root) {
  "use strict";
  var K = [0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];
  function rr(v,n) { return (v >>> n) | (v << (32-n)); }
  root.HEN_SHA256 = function (bytes) {
    var length=bytes.length, padded=new Uint8Array(Math.ceil((length+9)/64)*64);
    padded.set(bytes); padded[length]=0x80;
    var view=new DataView(padded.buffer), bitLength=length*8;
    view.setUint32(padded.length-8,Math.floor(bitLength/0x100000000),false);
    view.setUint32(padded.length-4,bitLength>>>0,false);
    var H=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19], W=new Int32Array(64);
    for(var offset=0;offset<padded.length;offset+=64) {
      var j; for(j=0;j<16;j++) W[j]=view.getUint32(offset+j*4,false);
      for(j=16;j<64;j++) {var x=W[j-15],y=W[j-2];W[j]=(W[j-16]+(rr(x,7)^rr(x,18)^(x>>>3))+W[j-7]+(rr(y,17)^rr(y,19)^(y>>>10)))|0;}
      var a=H[0],b=H[1],c=H[2],d=H[3],e=H[4],f=H[5],g=H[6],h=H[7];
      for(j=0;j<64;j++) {var t1=(h+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+K[j]+W[j])|0;var t2=((rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c)))|0;h=g;g=f;f=e;e=(d+t1)|0;d=c;c=b;b=a;a=(t1+t2)|0;}
      H[0]=(H[0]+a)|0;H[1]=(H[1]+b)|0;H[2]=(H[2]+c)|0;H[3]=(H[3]+d)|0;H[4]=(H[4]+e)|0;H[5]=(H[5]+f)|0;H[6]=(H[6]+g)|0;H[7]=(H[7]+h)|0;
    }
    var out="";for(var i=0;i<H.length;i++){var hex=(H[i]>>>0).toString(16);out+=("00000000"+hex).slice(-8);}return out;
  };
})(typeof window!=="undefined"?window:globalThis);
