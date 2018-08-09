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
BTLE.ready=function(event){
  if(this.link){
    this.emit(event);
    return;
  }
  else this.getS(event);
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
    await this.setWout(service);
    await this.setAds(service);
    this.link=true;
    this.emit(event);
//  this.writeAds(0xFC01);//request to send data
  }
  catch(error){
    this.emit('error',error);
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
    await this.setWout(service);
    await this.setAds(service);
    this.link=true;
    this.emit(event);
//    this.writeAds(0xFC01);//request to send data
  }
  catch(error){
    console.log('Argh! '+error);
    this.emit('error',error);
    this.device=null;
    this.link=false;
  }
}
BTLE.setWout=async function(service){
  console.log('Wout notification on '+service.hasOwnProperty('getCharacteristic'));
  let characteristic;
  try{
    characteristic=await service.getCharacteristic(this.charNotifUuid);
    await characteristic.startNotifications();
    this.emit('notif');
    let who=this;
    characteristic.addEventListener('characteristicvaluechanged',function(event){
//      who.emit('receive',event.target.value.toString());
      who.emit('receive',event.target.value);
    });
  }
  catch(error){
    console.log('Argh! '+error);
//    if(nchar!=null){
//    nchar.addEventListener('characteristicvaluechanged',notifCB);
//  }
	}
}
BTLE.setAds=async function(service){
  try{
    this.charAds=await service.getCharacteristic(this.charAdsUuid);
  }
  catch(error){
    $('#output').append('Argh! '+error+'<br>');
  }
}
BTLE.writeAds=async function(val){
  try{
    let data=new Uint8Array(2);
    data[0]=val>>8;
    data[1]=val;
    await this.charAds.writeValue(data);
  }
  catch(error){
    $('#output').append('Argh! '+error+'<br>');
  }
}
BTLE.writeDin=async function(service,val){
  $('#output').append('Write Data char...'+this.service+'<br>');
  try{
    const characteristic=await service.getCharacteristic(this.charDataUuid);
    const data=new Uint8Array(1);
    data[0]=val;
    await characteristic.writeValue(data);
  }
  catch(error){
    $('#output').append('Argh! '+error+'<br>');
  }
}

export default BTLE;