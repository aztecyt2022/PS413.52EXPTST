(function(){
  "use strict";
  var overlay=/(?:[?&])overlay=1(?:&|$)/.test(location.search||"");
  if(!window.HEN_AUTH.allowed()){
    location.replace("../index.html"+(overlay?"?overlay=1":""));
    return;
  }
  if(!window.HEN_DEVICE.supported){
    document.getElementById("msg").textContent="Disabled. Only a PS4 running firmware 13.52 is supported.";
    document.getElementById("state").textContent="Unsupported device";
    return;
  }
  var module=document.createElement("script");
  module.type="module";module.src="jb.js?v=22-pages-overlay-test1";
  document.head.appendChild(module);
})();
