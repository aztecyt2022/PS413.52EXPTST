(function(){
  "use strict";
  var build=window.HEN_BUILD;
  var diagnosticLink=document.getElementById("diagnostics");
  if(diagnosticLink)diagnosticLink.href="diagnostics.html"+build.query;
  if(!window.HEN_AUTH.allowed()){
    location.replace("../index.html"+build.query);
    return;
  }
  if(!window.HEN_DEVICE.supported){
    document.getElementById("msg").textContent="Disabled. Only PS4 firmware 13.52 is supported.";
    document.getElementById("state").textContent="Detected: "+(window.HEN_DEVICE.firmware||"not a PS4");
    return;
  }

  var message=document.getElementById("msg");

  var state=document.getElementById("state");
  var output=document.getElementById("out");
  var reported=false;
  window.HEN_LOADER_STARTED=false;

  function fail(stage, detail){
    if(reported || window.HEN_LOADER_STARTED)return;
    reported=true;
    if(document.body)document.body.className="log fail";
    if(message)message.textContent="Exploit loader did not start";
    if(state)state.textContent=stage;
    if(output)output.textContent+=(detail||"The loader could not initialize.")+"\n";
  }
  if(typeof window.addEventListener==="function"){
    window.addEventListener("error",function(event){
      if(window.HEN_LOADER_STARTED)return;
      var target=event.target;
      if(target && target.tagName==="SCRIPT"){
        fail("Script request failed",target.src||"A required JavaScript file could not load.");
      }else if(event.message){
        fail("JavaScript error",String(event.message).slice(0,240));
      }
    },true);
    window.addEventListener("unhandledrejection",function(event){
      if(!window.HEN_LOADER_STARTED){
        var reason=event.reason;
        fail("Module initialization failed",String(reason && reason.message||reason).slice(0,240));
      }
    });
  }
  var module=document.createElement("script");
  module.type="module";
  module.src="jb.js?v=dexcore-v1-current-only-1";
  module.onerror=function(){
    fail("Exploit module failed to load","Missing file, unsupported module syntax, or network error.");
  };
  document.head.appendChild(module);
  if(typeof setTimeout==="function"){
    setTimeout(function(){
      if(!window.HEN_LOADER_STARTED)
        fail("Exploit JavaScript never started","The PS4 browser may lack a required JavaScript feature or the module may be cached incorrectly. Clear website data, reload, and record this message.");
    },15000);
  }
})();
