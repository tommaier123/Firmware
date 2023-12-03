// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { WifiNetwork } from '../../../open-shock/serialization/types/wifi-network.js';


export class ReadyMessage {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):ReadyMessage {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsReadyMessage(bb:flatbuffers.ByteBuffer, obj?:ReadyMessage):ReadyMessage {
  return (obj || new ReadyMessage()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsReadyMessage(bb:flatbuffers.ByteBuffer, obj?:ReadyMessage):ReadyMessage {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new ReadyMessage()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

/**
 * Poggies is always true.
 */
poggies():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

/**
 * The radio transmit pin that the device is currently using.
 */
rftxPin():number {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
}

/**
 * Whether the device is linked to an account. (Has a valid access token)
 */
accountLinked():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

/**
 * The networks that the device has saved.
 */
networksSaved(index: number):string
networksSaved(index: number,optionalEncoding:flatbuffers.Encoding):string|Uint8Array
networksSaved(index: number,optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? this.bb!.__string(this.bb!.__vector(this.bb_pos + offset) + index * 4, optionalEncoding) : null;
}

networksSavedLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

/**
 * The network that the device is currently connected to.
 */
networkConnected(obj?:WifiNetwork):WifiNetwork|null {
  const offset = this.bb!.__offset(this.bb_pos, 12);
  return offset ? (obj || new WifiNetwork()).__init(this.bb!.__indirect(this.bb_pos + offset), this.bb!) : null;
}

static startReadyMessage(builder:flatbuffers.Builder) {
  builder.startObject(5);
}

static addPoggies(builder:flatbuffers.Builder, poggies:boolean) {
  builder.addFieldInt8(0, +poggies, +false);
}

static addRftxPin(builder:flatbuffers.Builder, rftxPin:number) {
  builder.addFieldInt8(1, rftxPin, 0);
}

static addAccountLinked(builder:flatbuffers.Builder, accountLinked:boolean) {
  builder.addFieldInt8(2, +accountLinked, +false);
}

static addNetworksSaved(builder:flatbuffers.Builder, networksSavedOffset:flatbuffers.Offset) {
  builder.addFieldOffset(3, networksSavedOffset, 0);
}

static createNetworksSavedVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startNetworksSavedVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addNetworkConnected(builder:flatbuffers.Builder, networkConnectedOffset:flatbuffers.Offset) {
  builder.addFieldOffset(4, networkConnectedOffset, 0);
}

static endReadyMessage(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

}
