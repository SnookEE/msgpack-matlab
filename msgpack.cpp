/* 
 * MessagePack for Matlab
 *
 * */

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <msgpack.hpp>

#include "mex.h"
#include "matrix.h"

mxArray* mex_unpack_boolean(msgpack::object obj);
mxArray* mex_unpack_positive_integer(msgpack::object obj);
mxArray* mex_unpack_negative_integer(msgpack::object obj);
mxArray* mex_unpack_double(msgpack::object obj);
mxArray* mex_unpack_raw(msgpack::object obj);
mxArray* mex_unpack_nil(msgpack::object obj);
mxArray* mex_unpack_map(msgpack::object obj);
mxArray* mex_unpack_array(msgpack::object obj);

typedef struct mxArrayRes mxArrayRes;
struct mxArrayRes
{
   mxArray * res;
   mxArrayRes * next;
};

#define DEFAULT_STR_SIZE 256
/* preallocate str space for unpack raw */
char *unpack_raw_str = (char *) mxMalloc(sizeof(char) * DEFAULT_STR_SIZE);

void (*PackMap[17])(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs);
mxArray* (*unPackMap[8])(msgpack::object  obj);

void mexExit(void)
{
//  mxFree((void *)unpack_raw_str);
   fprintf(stdout, "Existing Mex Msgpack \n");
   fflush(stdout);
}

mxArrayRes * mxArrayRes_new(mxArrayRes * head, mxArray* res)
{
   mxArrayRes * new_res = (mxArrayRes *) mxCalloc(1, sizeof(mxArrayRes));
   new_res->res = res;
   new_res->next = head;
   return new_res;
}

void mxArrayRes_free(mxArrayRes * head)
{
   mxArrayRes * cur_ptr = head;
   mxArrayRes * ptr = head;
   while (cur_ptr != NULL)
   {
      ptr = ptr->next;
      mxFree(cur_ptr);
      cur_ptr = ptr;
   }
}

mxArray* mex_unpack_boolean(msgpack::object  obj)
{
   return mxCreateLogicalScalar(obj.via.boolean);
}

mxArray* mex_unpack_positive_integer(msgpack::object obj)
{
   /*
    mxArray *ret = mxCreateNumericMatrix(1,1, mxUINT64_CLASS, mxREAL);
    uint64_t *ptr = (uint64_t *)mxGetPr(ret);
    *ptr = obj.via.u64;
    return ret;
    */
   return mxCreateDoubleScalar((double) obj.via.u64);
}

mxArray* mex_unpack_negative_integer(msgpack::object  obj)
{
   /*
    mxArray *ret = mxCreateNumericMatrix(1,1, mxINT64_CLASS, mxREAL);
    int64_t *ptr = (int64_t *)mxGetPr(ret);
    *ptr = obj.via.i64;
    return ret;
    */
   return mxCreateDoubleScalar((double) obj.via.i64);
}

mxArray* mex_unpack_double(msgpack::object  obj)
{
   /*
    mxArray* ret = mxCreateDoubleMatrix(1,1, mxREAL);
    double *ptr = (double *)mxGetPr(ret);
    *ptr = obj.via.dec;
    return ret;
    */
   return mxCreateDoubleScalar(obj.via.f64);
}

mxArray* mex_unpack_raw(msgpack::object  obj)
{
   /*
    mxArray* ret = mxCreateNumericMatrix(1,obj.via.raw.size, mxUINT8_CLASS, mxREAL);
    uint8_t *ptr = (uint8_t*)mxGetPr(ret);
    memcpy(ptr, obj.via.raw.ptr, obj.via.raw.size * sizeof(uint8_t));
    */
   if (obj.via.bin.size > DEFAULT_STR_SIZE)
      mxRealloc(unpack_raw_str, sizeof(char) * obj.via.bin.size);

   strncpy(unpack_raw_str, obj.via.bin.ptr, sizeof(char) * obj.via.bin.size);
   unpack_raw_str[obj.via.bin.size] = '\0';
   mxArray* ret = mxCreateString((const char *) unpack_raw_str);

   return ret;
}

mxArray* mex_unpack_nil(msgpack::object  obj)
{
   /*
    return mxCreateCellArray(0,0);
    */
   return mxCreateDoubleScalar(0);
}

mxArray* mex_unpack_map(msgpack::object  obj)
{
   uint32_t nfields = obj.via.map.size;
   char **field_name = (char **) mxCalloc(nfields, sizeof(char *));
   for (uint32_t i = 0; i < nfields; i++)
   {
      msgpack::object_kv obj_kv = obj.via.map.ptr[i];
      if (obj_kv.key.type == msgpack::v1::type::object_type::STR)
      {
         /* the raw size from msgpack only counts actual characters
          * but C char array need end with \0 */
         field_name[i] = (char*) mxCalloc(obj_kv.key.via.bin.size + 1, sizeof(char));
         memcpy((char*) field_name[i], obj_kv.key.via.bin.ptr, obj_kv.key.via.bin.size * sizeof(char));
      }
      else
      {
         mexPrintf("not string key\n");
      }
   }
   mxArray *ret = mxCreateStructMatrix(1, 1, obj.via.map.size, (const char**) field_name);
   msgpack_object ob;
   for (uint32_t i = 0; i < nfields; i++)
   {
      int ifield = mxGetFieldNumber(ret, field_name[i]);
      ob = obj.via.map.ptr[i].val;
      mxSetFieldByNumber(ret, 0, ifield, (*unPackMap[ob.type])(ob));
   }
   for (uint32_t i = 0; i < nfields; i++)
      mxFree((void *) field_name[i]);
   mxFree(field_name);
   return ret;
}

mxArray* mex_unpack_array(msgpack::object  obj)
{
   /* validata array element type */
   int types = 0;
   int unique_type = -1;
   for (size_t i = 0; i < obj.via.array.size; i++)
      if ((obj.via.array.ptr[i].type > 0) && (obj.via.array.ptr[i].type < 5) && (obj.via.array.ptr[i].type != unique_type))
      {
         unique_type = obj.via.array.ptr[i].type;
         types++;
      }
   if (types == 1)
   {
      mxArray *ret = NULL;
      bool * ptrb = NULL;
      double * ptrd = NULL;
      int64_t * ptri = NULL;
      uint64_t * ptru = NULL;
      switch (unique_type)
      {
      case 1:
         ret = mxCreateLogicalMatrix(1, obj.via.array.size);
         ptrb = (bool*) mxGetPr(ret);
         for (size_t i = 0; i < obj.via.array.size; i++)
            ptrb[i] = obj.via.array.ptr[i].via.boolean;
         break;
      case 2:
         ret = mxCreateNumericMatrix(1, obj.via.array.size, mxUINT64_CLASS, mxREAL);
         ptru = (uint64_t*) mxGetPr(ret);
         for (size_t i = 0; i < obj.via.array.size; i++)
            ptru[i] = obj.via.array.ptr[i].via.u64;
         break;
      case 3:
         ret = mxCreateNumericMatrix(1, obj.via.array.size, mxINT64_CLASS, mxREAL);
         ptri = (int64_t*) mxGetPr(ret);
         for (size_t i = 0; i < obj.via.array.size; i++)
            ptri[i] = obj.via.array.ptr[i].via.i64;
         break;
      case 4:
         ret = mxCreateNumericMatrix(1, obj.via.array.size, mxDOUBLE_CLASS, mxREAL);
         mexPrintf("Creating double matrix\n");
         ptrd = mxGetPr(ret);
         for (size_t i = 0; i < obj.via.array.size; i++)
            ptrd[i] = obj.via.array.ptr[i].via.f64;
         break;
      default:
         break;
      }
      return ret;
   }
   else
   {
      mxArray *ret = mxCreateCellMatrix(1, obj.via.array.size);
      for (size_t i = 0; i < obj.via.array.size; i++)
      {
         msgpack_object ob = obj.via.array.ptr[i];
         mxSetCell(ret, i, (*unPackMap[ob.type])(ob));
      }
      return ret;
   }
}

void mex_unpack(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   const char *str = (const char*) mxGetPr(prhs[0]);
   int size = mxGetM(prhs[0]) * mxGetN(prhs[0]);

   /* deserializes it. */
   msgpack::unpacked msg;
   msgpack::unpack(&msg,str,size);

   /* prints the deserialized object. */
   msgpack::object obj = msg.get();

   plhs[0] = (*unPackMap[obj.type])(obj);
}

void mex_pack_unknown(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   pk.pack_nil();
}

void mex_pack_void(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   pk.pack_nil();
}

void mex_pack_function(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   pk.pack_nil();
}

void mex_pack_single(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   float *data = (float*) mxGetPr(prhs);
//   if (nElements > 1)
//      msgpack_pack_array(pk, nElements);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
   {
      pk.pack(data[i]);
      //msgpack_pack_float(pk, data[i]);
   }
}

void mex_pack_double(msgpack::packer<msgpack::sbuffer> & pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   double *data = mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
   {
      pk.pack(data[i]);
   }
}

void mex_pack_int8(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   int8_t *data = (int8_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_uint8(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   uint8_t *data = (uint8_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_int16(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   int16_t *data = (int16_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_uint16(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   uint16_t *data = (uint16_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_int32(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   int32_t *data = (int32_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_uint32(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   uint32_t *data = (uint32_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_int64(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   int64_t *data = (int64_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_uint64(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   uint64_t *data = (uint64_t*) mxGetPr(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
      pk.pack(data[i]);
}

void mex_pack_logical(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   bool *data = mxGetLogicals(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
   {
      (data[i]) ? pk.pack_true() : pk.pack_false();
   }
}

void mex_pack_char(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   mwSize str_len = mxGetNumberOfElements(prhs) + 1;
   char *buf = (char *) mxCalloc(str_len, sizeof(char));

   if (mxGetString(prhs, buf, str_len) != 0)
      mexErrMsgTxt("Could not convert to C string data");

   pk.pack_str(str_len);
   pk.pack_str_body(buf,str_len);

   mxFree(buf);

   /* uint8 input
    int nElements = mxGetNumberOfElements(prhs);
    uint8_t *data = (uint8_t*)mxGetPr(prhs);
    */
   /* matlab char type is actually uint16 -> 2 * uint8 */
   /* uint8 input
    msgpack_pack_raw(pk, nElements * 2);
    msgpack_pack_raw_body(pk, data, nElements * 2);
    */
}

void mex_pack_cell(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nElements = mxGetNumberOfElements(prhs);
   if (nElements > 1)
      pk.pack_array(nElements);
   for (int i = 0; i < nElements; i++)
   {
      mxArray * pm = mxGetCell(prhs, i);
      (*PackMap[mxGetClassID(pm)])(pk, nrhs, pm);
   }
}

void mex_pack_struct(msgpack::packer<msgpack::sbuffer>& pk, int nrhs, const mxArray *prhs)
{
   int nField = mxGetNumberOfFields(prhs);
   if(nField > 1)
      pk.pack_map(nField);
   const char* fname = NULL;
   int fnameLen = 0;
   int ifield = 0;
   for (int i = 0; i < nField; i++)
   {
      fname = mxGetFieldNameByNumber(prhs, i);
      fnameLen = strlen(fname);
      pk.pack_str(fnameLen);
      pk.pack_str_body(fname,fnameLen);
      ifield = mxGetFieldNumber(prhs, fname);
      mxArray* pm = mxGetFieldByNumber(prhs, 0, ifield);
      (*PackMap[mxGetClassID(pm)])(pk, nrhs, pm);
   }
}

void mex_pack(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   /* creates buffer and serializer instance. */
   msgpack::sbuffer * buffer = new msgpack::sbuffer();
   msgpack::packer<msgpack::sbuffer> pk(buffer);

   for (int i = 0; i < nrhs; i++)
      (*PackMap[mxGetClassID(prhs[i])])(pk, nrhs, prhs[i]);

   plhs[0] = mxCreateNumericMatrix(1, buffer->size(), mxUINT8_CLASS, mxREAL);
   memcpy(mxGetPr(plhs[0]), buffer->data(), buffer->size() * sizeof(uint8_t));

   /* cleaning */
   delete buffer;

}

void mex_pack_raw(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   /* creates buffer and serializer instance. */
   msgpack::sbuffer * buffer = new msgpack::sbuffer();
   msgpack::packer<msgpack::sbuffer> pk(buffer);

   for (int i = 0; i < nrhs; i++)
   {
      size_t nElements = mxGetNumberOfElements(prhs[i]);
      size_t sElements = mxGetElementSize(prhs[i]);
      uint8_t *data = (uint8_t*) mxGetPr(prhs[i]);
      pk.pack_str(nElements * sElements);
      pk.pack_str_body((const char *)data,nElements * sElements);
   }

   plhs[0] = mxCreateNumericMatrix(1, buffer->size(), mxUINT8_CLASS, mxREAL);
   memcpy(mxGetPr(plhs[0]), buffer->data(), buffer->size() * sizeof(uint8_t));

   /* cleaning */
   delete buffer;

}

void mex_unpacker_set_cell(mxArray *plhs, int nlhs, mxArrayRes *res)
{
   if (nlhs > 0)
      mex_unpacker_set_cell(plhs, nlhs - 1, res->next);
   mxSetCell(plhs, nlhs, res->res);
}

void mex_unpacker(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   mxArrayRes * ret = NULL;
   int npack = 0;
   /* Init deserialize using msgpack_unpacker */
   msgpack::unpacker pac;
   pac.reserve_buffer(MSGPACK_UNPACKER_INIT_BUFFER_SIZE);

   const char *str = (const char*) mxGetPr(prhs[0]);
   int size = mxGetM(prhs[0]) * mxGetN(prhs[0]);
   if (size)
   {
      /* feeds the buffer */
      pac.reserve_buffer(size);
      memcpy(pac.buffer(), str, size);
      pac.buffer_consumed(size);

      /* start streaming deserialization */
      msgpack::unpacked msg;
      while(pac.next(&msg))
      {
         msgpack::object obj = msg.get();
         ret = mxArrayRes_new(ret, (*unPackMap[obj.type])(obj));
         npack++;
      }
      /* set cell for output */
      plhs[0] = mxCreateCellMatrix(npack, 1);
      mex_unpacker_set_cell((mxArray *) plhs[0], npack - 1, ret);
   }

   mxArrayRes_free(ret);
}

std::vector<mxArray *> cells;
void mex_unpacker_std(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   int npack = 0;
   /* Init deserialize using msgpack_unpacker */
   msgpack::unpacker pac;
   pac.reserve_buffer(MSGPACK_UNPACKER_INIT_BUFFER_SIZE);

   const char *str = (const char*) mxGetPr(prhs[0]);
   int size = mxGetM(prhs[0]) * mxGetN(prhs[0]);
   if (size)
   {
      /* feeds the buffer */
      pac.reserve_buffer(size);
      memcpy(pac.buffer(), str, size);
      pac.buffer_consumed(size);

      /* start streaming deserialization */
      msgpack::unpacked msg;
      while(pac.next(&msg))
      {
         msgpack::object obj = msg.get();
         cells.push_back((*unPackMap[obj.type])(obj));
         npack++;
      }
      /* set cell for output */
      plhs[0] = mxCreateCellMatrix(1, npack);
      for (size_t i = 0; i < cells.size(); i++)
      {
         mxSetCell(plhs[0], i, cells[i]);
      }
      cells.clear();
   }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   static bool init = false;
   /* Init unpack functions Map */
   if (!init)
   {
      unPackMap[MSGPACK_OBJECT_NIL]              = mex_unpack_nil;
      unPackMap[MSGPACK_OBJECT_BOOLEAN]          = mex_unpack_boolean;
      unPackMap[MSGPACK_OBJECT_POSITIVE_INTEGER] = mex_unpack_positive_integer;
      unPackMap[MSGPACK_OBJECT_NEGATIVE_INTEGER] = mex_unpack_negative_integer;
      unPackMap[MSGPACK_OBJECT_FLOAT]            = mex_unpack_double;
      unPackMap[MSGPACK_OBJECT_STR]              = mex_unpack_raw;
      unPackMap[MSGPACK_OBJECT_ARRAY]            = mex_unpack_array;
      unPackMap[MSGPACK_OBJECT_MAP]              = mex_unpack_map;

      PackMap[mxUNKNOWN_CLASS]  = mex_pack_unknown;
      PackMap[mxVOID_CLASS]     = mex_pack_void;
      PackMap[mxFUNCTION_CLASS] = mex_pack_function;
      PackMap[mxCELL_CLASS]     = mex_pack_cell;
      PackMap[mxSTRUCT_CLASS]   = mex_pack_struct;
      PackMap[mxLOGICAL_CLASS]  = mex_pack_logical;
      PackMap[mxCHAR_CLASS]     = mex_pack_char;
      PackMap[mxDOUBLE_CLASS]   = mex_pack_double;
      PackMap[mxSINGLE_CLASS]   = mex_pack_single;
      PackMap[mxINT8_CLASS]     = mex_pack_int8;
      PackMap[mxUINT8_CLASS]    = mex_pack_uint8;
      PackMap[mxINT16_CLASS]    = mex_pack_int16;
      PackMap[mxUINT16_CLASS]   = mex_pack_uint16;
      PackMap[mxINT32_CLASS]    = mex_pack_int32;
      PackMap[mxUINT32_CLASS]   = mex_pack_uint32;
      PackMap[mxINT64_CLASS]    = mex_pack_int64;
      PackMap[mxUINT64_CLASS]   = mex_pack_uint64;

      mexAtExit(mexExit);
      init = true;
   }

   if ((nrhs < 1) || (!mxIsChar(prhs[0])))
      mexErrMsgTxt("Need to input string argument");
   char *fname = mxArrayToString(prhs[0]);
   char *flag = new char[10];
   if (strcmp(fname, "pack") == 0)
   {
      if (mxIsChar(prhs[nrhs - 1]))
      {
         flag = mxArrayToString(prhs[nrhs - 1]);
      }
      if (strcmp(flag, "raw") == 0)
      {
         mex_pack_raw(nlhs, plhs, nrhs - 2, prhs + 1);
      }
      else
      {
         mex_pack(nlhs, plhs, nrhs - 1, prhs + 1);
      }
   }
   else if (strcmp(fname, "unpack") == 0)
   {
      mex_unpack(nlhs, plhs, nrhs - 1, prhs + 1);
   }
   else if (strcmp(fname, "unpacker") == 0)
   {
      mex_unpacker_std(nlhs, plhs, nrhs - 1, prhs + 1);
   }
   else
   {
      mexErrMsgTxt("Unknown function argument");
   }
}

