// ---------- lista de modulos ativos ----------
// UNICO arquivo que cada pessoa edita para ligar o proprio modulo: 1 linha por modulo.
// Para juntar os dois projetos: copie a pasta do modulo do seu amigo para src/modules/ e adicione a linha dele aqui.
// A ordem so importa se um modulo depender de outro (os ganchos rodam na ordem desta lista).
//
// #include "modules/exemplo/exemplo.inc"      // modulo de exemplo (/exemplo): copie a pasta como molde
#include "monitor/monitor.inc"   // Monitor Touch (navegador no bloco, sincronizado no PvP)
#include "jvm/aot_bridge.inc"   // patchJVM.5: pseudo Forge (AOT) ligado ao mundo
#include "jade/jade.inc"   // Jade (Hwyla): tooltip do alvo mirado
#include "treechop/treechop.inc"   // TreeChop: derruba a arvore inteira
#include "trashcans/trashcans.inc"   // Trash Cans (lixeiras)
#include "itemphysic/itemphysic.inc"   // ItemPhysic: itens deitados no chao
