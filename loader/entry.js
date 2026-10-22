(function(){
  "use strict";
  var build=window.HEN_BUILD;
  if(!window.HEN_AUTH.allowed()){
    location.replace("../index.html"+build.query);
    return;
  }
  if(!window.HEN_DEVICE.supported){
    document.getElementById("msg").textContent="Disabled. Only a PS4 running firmware 13.52 is supported.";
    document.getElementById("state").textContent="Unsupported device";
    return;
  }
  var module=document.createElement("script");
  module.type="module";module.src="jb.js?v=prohen220-build3-local";
  document.head.appendChild(module);
})();
