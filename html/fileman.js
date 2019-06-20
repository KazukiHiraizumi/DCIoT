import EventEmitter from "./EventEmitter.js";

let storage=localStorage;
let FileMan=new EventEmitter();
FileMan.render=function(){
	let evptr=this;
	$('#FMload').click(function(e){
		evptr.loadEv(e);
	});
	$('#FMload').html('&nabla;');
	$('#FMsave').click(function(e){
		evptr.saveEv(e);
	});
	$('#FMsave').html('&nabla;');
	$('#FMremove').click(function(e){
		evptr.removeEv(e);
	});
	$('#FMremove').html('&nabla;');
	this.seled=false;
  this.fontsize=$('#FMload').css('font-size');
}
FileMan.sel=function(obj){
  obj.html('X');
  obj.css({background:'cyan','font-size':'larger'});
  this.seled=true;
}
FileMan.unsel=function(obj){
  $('#FMpop').empty();
  obj.html('&nabla;');
  obj.css({background:'','font-size':this.fontsize});
  this.seled=false;
}
FileMan.unselall=function(){
  $('#FMpop').empty();
  $('#FMload').html('&nabla;').css({background:'','font-size':this.fontsize});
  $('#FMsave').html('&nabla;').css({background:'','font-size':this.fontsize});
  $('#FMremove').html('&nabla;').css({background:'','font-size':this.fontsize});
  this.seled=false;
}
FileMan.name="";
FileMan.check=function(obj){
  if(obj.html()=='X'){
    this.unsel(obj);
    return true;
  }
  else{
    this.unselall();
    return false;
  }
}
FileMan.json_ls=function(callback){
  let list=[];
  for(let i=0;;i++){
    let key=storage.key(i);
    console.log("key="+key);
    if(key==null) break;
    list.push(key);
  }
  if(list.length>0) callback(list);
}
FileMan.loadEv=function(e){
  let target=$(e.target);
  if(this.check(target)) return;
  let evptr=this;
  this.json_ls(function(list){
    evptr.sel(target);
    $('#FMpop').append('<div>ファイル読み込み</div>');
		for(let i=0;i<list.length;i++){
		  $('#FMpop').append('<a href="#">'+list[i]+'</a>&nbsp;&nbsp;&nbsp;&nbsp;');
		}
		$('#FMpop a').click(function(e){
      evptr.name=$(e.target).text();
      evptr.json_load(evptr.name,function(dat){
  		  evptr.reflect(dat);
  		  evptr.unsel(target);
      });
		});
	});
}
FileMan.removeEv=function(e){
  let target=$(e.target);
	if(this.check(target)) return;
	let evptr=this;
	this.json_ls(function(list){
		evptr.sel(target);
    $('#FMpop').append('<div>ファイル削除</div>');
		for(let i=0;i<list.length;i++){
		  $('#FMpop').append('<a href="#">'+list[i]+'</a>&nbsp;&nbsp;&nbsp;&nbsp;');
		}
		$('#FMpop a').click(function(e){
			let fn=$(e.target).text();
			if(confirm('! ファイル '+fn+' を削除します')){
				evptr.json_rm($(e.target).text());
				evptr.unsel(target);
			}
		});
		if($('#FMpop a').length==0){
			evptr.unsel(target);
		}
	});
}
FileMan.saveEv=function(e){
  let target=$(e.target);
  if(this.check(target)) return;
	let evptr=this;
	let ovrwrt=true;
	this.json_ls(function(list){
	  evptr.sel(target);
//		$('#FMpop').html('<div>ファイル名</div><div><input style="width:75%;"></input><button>&radic;</button></div>');
		$('#FMpop').append('<div>ファイル名</div>');
		$('#FMpop').append('<div><input style="width:75%;"></input><button>&radic;</button></div>');
		let obj=evptr.collect();
	  $('#FMpop input').val(evptr.name);
		$('#FMpop input').focus();
		$('#FMpop input').css('color','red');
		$('#FMpop button').click(function(){
			if(ovrwrt && !confirm('ファイルを上書きします')) return;
			let nam=$('#FMpop input').val();
			if(nam!=''){
        evptr.name=nam;
				evptr.json_save(nam,obj);
			  evptr.unsel(target);
			}
		});
		$('#FMpop input').keyup(function(){
			if(list.indexOf($('#FMpop input').val())<0){
				$('#FMpop input').css('color','black');
				ovrwrt=false;
			}
			else{
				$('#FMpop input').css('color','red');
				ovrwrt=true;
			}
		});
		$('#FMpop input').keypress(function(event){
			if(event.which==13){
				if(ovrwrt && !confirm('ファイルを上書きします')) return;
				var nam=$('#FMpop input').val();
				if(nam!=''){
					evptr.name=nam;
					evptr.json_save(nam,obj);
					evptr.unsel(target);
				}
			}
		});
	});
}
FileMan.json_save=function(name,data){
	storage.setItem(name,JSON.stringify(data));
}
FileMan.json_load=function(name,callback){
	let val=storage.getItem(name);
  let dat=JSON.parse(val);
  console.log("dat="+val);
  callback(dat);
}
FileMan.json_rm=function(name){
	storage.removeItem(name);
}


export default FileMan;