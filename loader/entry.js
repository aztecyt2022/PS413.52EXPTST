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
  var state=document.getElementById("state");
  if(state)state.textContent="Loading exploit JavaScript modules…";
  function fail(message){
    var title=document.getElementById("msg");
    var out=document.getElementById("out");
    if(title)title.textContent="Exploit loader error";
    if(state)state.textContent=message;
    if(out)out.textContent += (out.textContent?"\\n":"")+message;
    if(document.body)document.body.className="log fail";
  }
  if(window.addEventListener){
    window.addEventListener("error",function(event){
      var target=event.target||{};
      fail(event.message || (target.src?"Unable to load: "+target.src:"Unexpected loader script error"));
    },true);
    window.addEventListener("unhandledrejection",function(event){
      var reason=event.reason;
      fail("Loader promise rejected: "+(reason&&reason.message?reason.message:String(reason)));
    });
  }
  var module=document.createElement("script");
  module.type="module";
  module.src="jb.js?v=henacy-v1-static-host-diagnostics";
  module.onerror=function(){fail("Could not load exploit module or a dependency. Clear PS4 browser website data, then reload.");};
  document.head.appendChild(module);
  if(typeof setTimeout==="function")setTimeout(function(){
    if(window.HENACY_LOADER_STARTED!==true)
      fail("Exploit module has not started. Check PS4 browser module support, connection and cached files.");
  },20000);
})();
