//Dependency: jquery
$(document).ready(function(){


	var cssclass="rps";
	var config={
		url:window.location.origin+window.location.pathname,
		graphdatabufsz:100,
		logdatabufsz:8,
		graphupdateperiodms:1000,
		running:false
	};
	window.config=config;
	url2cfg();
	//fn predecl
	var log;
	var exec;
	var show_config;
	var hide_config;
	//--fn predecl
	var idx=0;
	var graphdata=[];
	var logdata=[];for(var i=0;i<config.logdatabufsz;i++)logdata.push("");
	var stat={
		t0:new Date(),
		idx:0,
		idxs:0,
		rps:0
	};
function pad(pad, str, padLeft) {
  if (typeof str === 'undefined') 
    return pad;
  if (padLeft) {
    return (pad + str).slice(-pad.length);
  } else {
    return (str + pad).substring(0, pad.length);
  }
}
	function addgraphdata(val){
		graphdata.push(val);
		if(graphdata.length>config.graphdatabufsz)
			graphdata=graphdata.slice(graphdata.length-config.graphdatabufsz,graphdata.length);
	}
	function start(){
		if(!config.running){
			log("STARTING...");
			stat.idx=0;
			config.running=true;
			exec();
			cfg2url();
		}else{
		}
	};
	function stop(){
		if(config.running){
			config.running=false;
			cfg2url();
			log("STOPPING...");
		}
	};
	function hdlAjaxSuccess(){
		stat.idx++;
		stat.idxs++;
		var t=new Date();
		var tdiff=t-stat.t0
		if(tdiff>config.graphupdateperiodms){
			stat.rps=config.graphupdateperiodms*stat.idxs/tdiff;
			stat.t0=t;
			addgraphdata(stat.rps);
			updateGraph();
			log([
				pad('.'.repeat(16),"rps:"+(Math.round(stat.rps*100))/100,false),
				pad('.'.repeat(8),"rdx:"+stat.idxs,false),
				pad('.'.repeat(16),"sdx:"+stat.idx,false),
			].join(""));
			stat.idxs=0;
		}
		if(config.running)exec();
	};
	function hdlAjaxError(){
		hdlAjaxSuccess();
	};
	function exec(){
		$.ajax({
			url:config.url,
			context:document.body,
			success:function(returnData){
				hdlAjaxSuccess();
			},
			error:function(xhr,status,error){
				hdlAjaxError();
			}
		});
	}
	function cfg2url(){
		try{
			var origloc=window.location.pathname;//+window.location.search;
			origloc+="?rps="+btoa(JSON.stringify(config));
			window.history.pushState("","",origloc);
		}catch(e){console.error(e);}
	}
	function url2cfg(){
		const urlParams = new URLSearchParams(location.search);
		for (const [key, value] of urlParams) {
		    if(key=="rps")try{
				var cfgstr=atob(value);
				var urlconfig=JSON.parse(cfgstr);
				Object.keys(urlconfig).forEach(function(cfgk){
					config[cfgk]=urlconfig[cfgk];
				})
			}catch(e){console.error(e)};
		}
	}

	var style=$("<style/>").text(`
		.${cssclass}{
			position:relative;
			padding:8px;
			position:absolute;
			width:320px;
			height:240px;
			background:#000000;
			color:#FFFFFF;
		}
		.${cssclass} button{
			background:#444444;
			color:#888888;
			border:unset;
			margin:0px;
			font-family:Monospace;
			font-weight:bold!important;
		}
		.${cssclass} .lbl{
			font-size:12px;
			color:#888888;
			font-family:Monospace;
			margin-right:8px;
			font-weight:bold;
		}
		.${cssclass} #graph{
			position:relative;
			margin:8px 0px 0px 0px;
			border:1px solid #888888;
			padding:0px;
			padding-left:20px;
			background:#222222;
			height:100px;

			background-size: 25px 25px!important;
			background-image:linear-gradient(to right, grey 1px, transparent 1px),linear-gradient(to bottom, grey 1px, transparent 1px)!important;

		}

		.${cssclass} #graph>#lbl_max{
			position:absolute;
			left:4px;
			top:4px;
		}
		.${cssclass} #graph>#lbl_min{
			position:absolute;
			left:4px;
			bottom:4px;
		}
		.${cssclass} #graph>div{
			vertical-align:bottom;
			display:inline-block;
		}
		.${cssclass} #log{
			font-size:10px;
			background:#222222;
			color:#888888;
			border:0px;
			padding:4px;
			width:97%;
		}
		.${cssclass} #config{
			position:absolute;
			width:100%;
			height:100%;
			top:0px;
			left:0px;
			background:rgba(0,0,0,0.8);
		}
		.${cssclass} #config>.container{
			padding:8px;
		}
		.${cssclass} #config input{
			background:#444444;
			color:#888888;
			border:0px;
			padding:4px;
			width:90%;
		}
	`);
	var div_rps=$("<div/>").addClass(cssclass);
	var div_main=$("<div/>");
	var div_graph=$("<div/>").attr({"id":"graph"});
	var div_log=$("<pre/>").attr({"id":"log"});
	var div_ctl=$("<div/>").attr({"id":"ctl"});
	var div_config=$("<div/>").attr({"id":"config"}).hide();

	div_rps.append(div_main);
	div_rps.append(div_config);
	div_rps.append(style);
	div_main.append(div_ctl);
	div_main.append(div_graph);
	div_main.append(div_log);

	var btn_start=$("<button/>").attr({"id":"btn_start"}).text("START").click(start);
	var btn_stop=$("<button/>").attr({"id":"btn_start"}).text("STOP").click(stop);
	var btn_config=$("<button/>").attr({"id":"btn_config"}).text("CONFIG").click(show_config);

	div_ctl.append(btn_start);
	div_ctl.append(btn_stop);
	div_ctl.append(btn_config);

	//config
	{
		var div_container=$("<div/>").addClass("container");
		var btn_back=$("<button/>").text("BACK").click(hide_config);
		div_container.append(btn_back);
		div_container.append($("<br/>"));
		{
			var input_url=$("<input/>").val(config.url);
			div_container.append($("<span/>").addClass("lbl").text("URL:"));
			div_container.append(input_url);
			input_url.keyup(function(){
				config.url=($(this).val());
				cfg2url();
			});
			div_container.append($("<br/>"));
		}
		{
			var input_url=$("<input/>").val(config.graphupdateperiodms);
			div_container.append($("<span/>").addClass("lbl").text("Update Period[ms]:"));
			div_container.append(input_url);
			input_url.keyup(function(){
				//graphdatabufsz:100,
				try{
					var val=eval($(this).val());
					if(typeof(val)=="number")
					config.graphupdateperiodms=val;
				}catch(e){
				}
				$(this).val(config.graphupdateperiodms)
				cfg2url();
			});
			div_container.append($("<br/>"));
		}
		{
			var input_url=$("<input/>").val(config.graphdatabufsz);
			div_container.append($("<span/>").addClass("lbl").text("Graph Buffer Size:"));
			div_container.append(input_url);
			input_url.keyup(function(){
				try{
					var val=eval($(this).val());
					if(typeof(val)=="number")
					config.graphdatabufsz=Math.round(val);
				}catch(e){
				}
				$(this).val(config.graphdatabufsz)
				cfg2url();
			});
			div_container.append($("<br/>"));
		}

		div_config.append(div_container);
	}
	//--config

	function log(val){
		logdata.push(val);
		if(logdata.length>config.logdatabufsz)
			logdata=logdata.slice(logdata.length-config.logdatabufsz,logdata.length);
		div_log.text(Array.from(logdata).reverse().join("\n"))
	}
	function updateGraph(){
		div_graph.empty();
		var maxval=0;
		graphdata.forEach(function(val){
			maxval=maxval<val?val:maxval;
		});
		maxval=Math.floor((maxval*100))/100;
		div_graph.append($("<span/>").text(maxval).addClass("lbl").attr({"id":"lbl_max"}));
		div_graph.append($("<span/>").text(0).addClass("lbl").attr({"id":"lbl_min"}));
		graphdata.forEach(function(val){
			var val_=val/maxval*100;
			div_graph.append($(
				"<div/>"
			).css({
				"height":`${val_}%`,
				"width":`${100/config.graphdatabufsz}%`,
				//"background":`rgba(255,255,255,${val_/100})`,
				"background":`hsl(${val_},100%,50%)`,
			}).text("")
			);
		});
	};
	function show_config(val){
		div_config.show();
	}
	function hide_config(val){
		div_config.hide();
	}
	$("body").append(div_rps);
	log("READY>");
	if(config.running){config.running=false;start();}
});
