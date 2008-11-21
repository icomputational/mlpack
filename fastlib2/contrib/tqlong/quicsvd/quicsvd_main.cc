/**
 * @file quicsvd_main.cc
 *
 * This file implements command line interface for the QUIC-SVD
 * method. It approximate the original matrix by another matrix
 * with smaller dimension to a certain accuracy degree specified by the 
 * user and then make SVD decomposition in the projected supspace.
 *
 * Run with --help for more usage.
 * 
 * @see quicsvd.h
 */

#include <fastlib/fastlib.h>
#include "quicsvd.h"

const fx_entry_doc quicsvd_main_entries[] = {
  {"A_in", FX_REQUIRED, FX_STR, NULL,
   " File consists of matrix A to be decomposed A = U S VT. \n"},
  {"relErr", FX_PARAM, FX_DOUBLE, NULL,
   " Target relative error |A-A'|/|A|, default = 0.1.\n"},
  {"U_out", FX_PARAM, FX_STR, NULL,
   " File to hold matrix U.\n"},
  {"s_out", FX_PARAM, FX_STR, NULL,
   " File to hold the singular values vector s.\n"},
  {"VT_out", FX_PARAM, FX_STR, NULL,
   " File to hold matrix VT (V transposed).\n"},
  {"quicsvd_time", FX_TIMER, FX_CUSTOM, NULL,
   " time to run the QUIC-SVD algorithm.\n"},
  {"lasvd_time", FX_TIMER, FX_CUSTOM, NULL,
   " time to run the SVD algorithm from LAPACK.\n"},
  FX_ENTRY_DOC_DONE
};

const fx_submodule_doc quicsvd_main_submodules[] = {
  FX_SUBMODULE_DOC_DONE
};

const fx_module_doc quicsvd_main_doc = {
  quicsvd_main_entries, quicsvd_main_submodules,
  "This is a program calculating an approximated Singular "
  "Value Decomposition using QUIC-SVD method.\n"
};

int main(int argc, char* argv[]) {
  fx_init(argc, argv, &quicsvd_main_doc);

  Matrix A, U, VT;
  Vector s;

  // parse input file to get matrix A
  const char* A_in = fx_param_str(NULL, "A_in", NULL);

  printf("Loading data ... ");
  data::Load(A_in, &A);
  printf("done.\n");

  // parse target relative error, default = 0.1
  const double targetRelErr = fx_param_double(NULL, "relErr", 0.1);

  printf("QUIC-SVD start ... ");
  fx_timer_start(NULL, "quicsvd_time");
  // call the QUIC-SVD method
  QuicSVD::SVDInit(A, targetRelErr, &s, &U, &VT);
  fx_timer_stop(NULL, "quicsvd_time");
  printf("stop.\n");
  
  if (fx_param_exists(NULL, "U_out"))
    data::Save(fx_param_str(NULL, "U_out", NULL), U);
  else // use OT to write to standard output
    ot::Print(U, "U", stdout);

  if (fx_param_exists(NULL, "s_out")) {
    Matrix S;
    S.AliasColVector(s);
    data::Save(fx_param_str(NULL, "s_out", NULL), S);
  }
  else 
    ot::Print(s, "s", stdout);

  if (fx_param_exists(NULL, "VT_out"))
    data::Save(fx_param_str(NULL, "VT_out", NULL), VT);
  else 
    ot::Print(VT, "VT", stdout);

  s.Destruct();
  U.Destruct();
  VT.Destruct();
  printf("LAPACK-SVD start ... ");
  fx_timer_start(NULL, "lasvd_time");
  // call the QUIC-SVD method
  la::SVDInit(A, &s, &U, &VT);
  fx_timer_stop(NULL, "lasvd_time");
  printf("stop.\n");

  fx_done(NULL);
}
