(function () {
  "use strict";
  var key="hen-pages-login-v1";
  var lifetime=8*60*60*1000;
  function allowed(){
    try{var until=Number(sessionStorage.getItem(key));return isFinite(until)&&until>Date.now()&&until<=Date.now()+lifetime;}
    catch(_){return false;}
  }
  function enter(password){
    if(typeof password!=="string"||password.length>128)return "password";
    try{
      var text=unescape(encodeURIComponent(password));
      var bytes=new Uint8Array(text.length);
      for(var i=0;i<text.length;i++)bytes[i]=text.charCodeAt(i);
      if(window.HEN_SHA256(bytes)!==window.HEN_CONFIG.passwordHash)return "password";
      sessionStorage.setItem(key,String(Date.now()+lifetime));
      return allowed()?"ok":"storage";
    }catch(_){return "storage";}
  }
  function logout(){try{sessionStorage.removeItem(key);}catch(_){};}
  window.HEN_AUTH={allowed:allowed,enter:enter,logout:logout};
})();
