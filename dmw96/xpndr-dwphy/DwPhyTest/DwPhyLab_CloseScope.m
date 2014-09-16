function DwPhyLab_CloseScope(h)

h.Disconnect;
delete(h); 
clear h;
