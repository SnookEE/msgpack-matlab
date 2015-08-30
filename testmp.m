clear msgpack;
msgpack('unpack', msgpack('pack', 'hello'))

msg = msgpack('pack', uint8('hello'))
char(msgpack('unpack', msg))

msg_id = 21312;
data   = zeros(600,2);

msg = msgpack('pack',uint32(msg_id),data);
obj = msgpack('unpacker', msg) 

clear msgpack;

%%
filename = '~/Downloads/imuPrunedMP-03.16.2013.15.30.15-0';
%
fid = fopen(filename);
data = fread( fid , '*uint8');

fclose(fid);

a = msgpack('unpacker', data);

sct = a{1}
msgpack('pack', sct)

%msg = msgpack('pack', [1,432,43.4]);
%%msg = msgpack('pack', 'fafafaeew23f');
%a = msgpack('unpack', msg)
%%filename = 'msg1';
%%fid = fopen(filename, 'w');
%%data = fwrite(fid, msg, '*uint8');
%%fclose(fid);
%
%msg = msgpack('pack', int16(-32));
%class(msgpack('unpack', msg));
