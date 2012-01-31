(* The Caml Light toplevel system. Main loop *)

#open "misc";;
#open "location";;
#open "do_phr";;
#open "compiler";;
#open "format";;
#open "readline";;

let mainloop() =
  try
    sys__catch_break true;
    readline__readline_init ();
    let fill_buff_fun = (fun buf n -> let s = (readline "#") in
                              let l = string_length s in
                                  let m=(if l>n then n else l) in
                                      (blit_string s 0 buf 0 m;m)
    ) in
    let lexbuf = lexing__create_lexer fill_buff_fun in
    input_lexbuf := lexbuf;
    while true do
      try
        reset_rollback();
        do_toplevel_phrase(parse_impl_phrase lexbuf)
      with End_of_file ->
             print_newline ();
             io__exit 0
         | Toplevel ->
             flush std_err;
             rollback ()
         | sys__Break ->
             print_string(interntl__translate "Interrupted.\n");
             rollback ()
    done

with sys__Sys_error msg ->
      interntl__eprintf "Input/output error: %s.\n" msg;
      exit 2
   | Zinc s ->
      interntl__eprintf "Internal error: %s.\nPlease report it.\n" s;
      exit 100
;;

printexc__f mainloop (); exit 0;;
