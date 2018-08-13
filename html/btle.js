import EventEmitter from "./EventEmitter.js";

const ab2str = function(buf) {
  let bufView = new Uint8Array(buf);
  return String.fromCharCode.apply(null, bufView);
}
const str2ab = function(str) {
  let encodedString = unescape(encodeURIComponent(str));
  let bytes = new Uint8Array(encodedString.length);
  for (let i=0;i<encodedString.length;++i){
    bytes[i]=encodedString.charCodeAt(i);
  }
  return bytes.buffer;
}

let BTLE=new EventEmitter();
BTLE.device=null;
BTLE.link=false;
BTLE.discon=function(){
  if(this.device==null){
    console.log('No GATT to disconnect');
  }
  else{
    this.device.gatt.disconnect();
    this.link=false;
  }
}
BTLE.ready=function(){
  if(this.link){
    this.emit('ready');
    return;
  }
  else this.getS('ready');
}
BTLE.getS=async function(event){
  if(this.device!=null){
    this.reconS(event);
    return;
  }
  try{
    console.log('Requesting Bluetooth Device');
    this.device=await navigator.bluetooth.requestDevice({
      filters: [{name: 'Shimano'}],
      optionalServices: [this.serviceUuid]
    });
    console.log('Device: '+this.device);
    this.emit('connect');
    const who=this;
    this.device.addEventListener('gattserverdisconnected',function(event){
      who.link=false;
      console.log('Device disconnected');
      who.emit('disconnect');
    });
    const server=await this.device.gatt.connect();
    this.emit('server');
    const service=await server.getPrimaryService(this.serviceUuid);
    this.emit('service');
    await this.setAds(service);
    await this.setWout(service);
    this.link=true;
    this.emit(event);
  }
  catch(error){
    this.emit('error',error);
    this.discon();
    this.device=null;
    this.link=false;
    console.log('Argh! '+error);
  }
}
BTLE.reconS=async function(event){
  try{
    const server= await this.device.gatt.connect();
    this.emit('connect');
    this.emit('server');
    const service=await server.getPrimaryService(this.serviceUuid);
    this.emit('service');
    await this.setAds(service);
    await this.setWout(service);
    this.link=true;
    this.emit(event);
  }
  catch(error){
    console.log('Argh! '+error);
    this.emit('error',error);
    this.discon();
    this.device=null;
    this.link=false;
  }
}
BTLE.setWout=async function(service){
  console.log('Wout notification on '+service.hasOwnProperty('getCharacteristic'));
  let characteristic=await service.getCharacteristic(this.charNotifUuid);
  await characteristic.startNotifications();
  this.emit('woutok');
  let who=this;
  characteristic.addEventListener('characteristicvaluechanged',function(event){
    who.emit('receive',event.target.value);
  });
}
BTLE.setAds=async function(service){
  this.charAds=await service.getCharacteristic(this.charAdsUuid);
  this.emit('adsok');
}
BTLE.write2=async function(cmd){
  try{
    let data=new Uint8Array(2);
    data[0]=cmd>>8;
    data[1]=cmd;
    await this.charAds.writeValue(data);
  }
  catch(error){
    console.log('write2 error:'+error);
    this.emit('error',error);
    this.discon();
    this.device=null;
  }
}
BTLE.write3=async function(cmd,val){
  try{
    let data=new Uint8Array(3);
    data[0]=cmd>>8;
    data[1]=cmd;
    data[2]=val;
    await this.charAds.writeValue(data);
  }
  catch(error){
    console.log('write3 error:'+error);
    this.emit('error',error);
    this.discon();
    this.device=null;
  }
}

export default BTLE;