IRC = Internet Relay Chat
C'est un système de chat en temps réel inventé en 1988.

Commande
Description
Exemple



PASS
S'authentifier
PASS secret123


NICK
Définir son pseudo
NICK alice


USER
Définir son username
USER alice 0 * :Alice


JOIN
Rejoindre un canal
JOIN #general


PRIVMSG
Envoyer un message
PRIVMSG #general :Salut!


QUIT
Se déconnecter
QUIT :Bye!


Commandes opérateur

KICK #canal bob :raison
INVITE bob #canal
TOPIC #canal :nouveau sujet
MODE #canal +i (invite-only)
MODE #canal +t (topic restreint)
MODE #canal +k pass (mot de passe)
MODE #canal +o bob (donner opérateur)
MODE #canal +l 10 (limite utilisateurs)






# Terminal 1 : lancer ton serveur
./ircserv 6667 pass123

# Terminal 2 : se connecter avec nc
nc 127.0.0.1 6667
PASS pass123
NICK alice
USER alice 0 * :Alice
JOIN #test
PRIVMSG #test :Hello world




📝 Checklist finale (ce que tu DOIS savoir)
✅ Authentification

 Expliquer l'ordre PASS → NICK → USER
 Dire ce qui se passe si mauvais mot de passe
 Expliquer tryRegister()

✅ Canaux

 Expliquer comment on crée un canal
 Dire qui devient opérateur (le créateur)
 Expliquer la différence entre members et operators

✅ Commandes

 Tracer un JOIN complet (avec tous les checks)
 Expliquer PRIVMSG canal vs PRIVMSG user
 Dire pourquoi on broadcast AVANT de retirer dans PART/KICK

✅ Modes

 Expliquer chaque mode (+i, +t, +k, +o, +l)
 Dire quels modes nécessitent un paramètre
 Expliquer pourquoi seuls les ops peuvent changer les modes







 # 📋 PLAN D'APPRENTISSAGE SIMPLIFIÉ - Partie LOGIQUE

Vu l'organisation des fichiers, voici **exactement** ce que tu dois apprendre dans l'ordre.

---

## 🎯 **TON PÉRIMÈTRE (Partie 2 : Logique métier)**

```
TU DOIS MAÎTRISER CES FICHIERS :
├─ include/Channel.hpp          ← Structure des canaux
├─ include/Client.hpp           ← Structure des clients
├─ include/Numerics.hpp         ← Codes de réponse IRC (ERR_XXX, RPL_XXX)
├─ src/Handlers_Core.cpp        ← PASS, NICK, USER (authentification)
├─ src/Handlers_Channel.cpp     ← JOIN, PART, KICK, INVITE, TOPIC
├─ src/Handlers_Channel_Modes.cpp ← MODE (+i, +t, +k, +o, +l)
└─ src/Handlers_Extras.cpp      ← PRIVMSG, NOTICE, etc.
```

**TON MATE SE CHARGE DE :**
```
├─ include/Server.hpp           ← Déclarations générales
├─ include/Poller.hpp           ← Wrapper poll()
├─ include/Parser.hpp           ← Parsing des lignes IRC
├─ src/Server.cpp               ← Setup du serveur
├─ src/ServerIO.cpp             ← accept/read/write
├─ src/ServerDispatch.cpp       ← Dispatcher qui appelle les handlers
├─ src/Poller.cpp               ← Implémentation poll()
├─ src/Parser.cpp               ← Parsing
└─ src/main.cpp                 ← Point d'entrée
```

---

## 📚 **TON PLAN D'APPRENTISSAGE (6 étapes)**

### **Étape 1 : Comprendre les structures (1h)**

#### **1.1 Lire `include/Client.hpp`**
```cpp
// Ce que tu dois retenir :
struct Client {
    int fd;                  // File descriptor
    std::string nick;        // Pseudo
    std::string user;        // Username
    std::string realname;    // Nom complet
    bool passwordOk;         // PASS validé ?
    bool registered;         // Enregistrement complet ?
    // ... + buffers
};
```

**QUESTION À TE POSER :** Quand `registered` passe-t-il à `true` ?

---

#### **1.2 Lire `include/Channel.hpp`**
```cpp
// Ce que tu dois retenir :
struct Channel {
    std::string name;                // "#test"
    std::string topic;               // Sujet du canal
    std::set<int> members;           // Tous les membres (fd)
    std::set<int> operators;         // Les opérateurs (fd)
    
    // Modes
    bool inviteOnly;      // +i
    bool topicRestricted; // +t
    std::string key;      // +k
    int userLimit;        // +l
    std::set<std::string> inviteList; // Pour mode +i
};
```

**QUESTION À TE POSER :** Quelle est la différence entre `members` et `operators` ?

---

#### **1.3 Parcourir `include/Numerics.hpp`**

**Juste comprendre le principe :**
```cpp
#define RPL_WELCOME 001      // Message de bienvenue
#define ERR_NOSUCHNICK 401   // Utilisateur inconnu
#define ERR_NOSUCHCHANNEL 403 // Canal inconnu
#define ERR_NOTONCHANNEL 442 // Tu n'es pas dans ce canal
// etc.
```

**PAS BESOIN DE MÉMORISER** : tu peux les consulter pendant la correction !

---

### **Étape 2 : Authentification (2h) - `src/Handlers_Core.cpp`**

**Ouvre ce fichier et analyse ces 3 fonctions :**

#### **2.1 `h_PASS()` (30min)**
```
Objectif : Vérifier le mot de passe du serveur

Flow :
1. Vérifier qu'il y a un paramètre
2. Vérifier que pas déjà registered
3. Comparer avec le mot de passe du serveur
4. Si mauvais → disconnect()
   Si bon → passwordOk = true
```

**EXERCICE :** Sur papier, trace :
```
Client envoie : "PASS wrongpassword"
→ Que se passe-t-il ?

Client envoie : "PASS secret" (bon mdp)
→ Que se passe-t-il ?
```

---

#### **2.2 `h_NICK()` (1h)**
```
Objectif : Définir/changer le pseudo

Flow :
1. Vérifier qu'il y a un paramètre
2. Vérifier que le nick est valide
3. Vérifier que le nick n'est pas déjà pris
4. Si déjà registered → notifier tous les canaux du changement
5. Mettre à jour client.nick et nickToFd map
6. Appeler tryRegister()
```

**EXERCICE :** Trace ces 2 scénarios :

**Scénario 1 : Premier nick**
```
Client (fd=4) : "NICK alice"
→ clients[4].nick = "alice"
→ nickToFd["alice"] = 4
→ tryRegister(4) appelé
```

**Scénario 2 : Nick déjà pris**
```
Client 1 (fd=4) : "NICK alice"  → OK
Client 2 (fd=5) : "NICK alice"  → ERR_NICKNAMEINUSE
```

---

#### **2.3 `h_USER()` et `tryRegister()` (30min)**
```
h_USER() :
1. Vérifier paramètres
2. Sauvegarder user et realname
3. Appeler tryRegister()

tryRegister() :
1. Vérifier que passwordOk && !nick.empty() && !user.empty()
2. Si oui → registered = true + envoyer RPL_WELCOME
```

**EXERCICE :** Trace l'ordre complet :
```
1. PASS secret     → passwordOk = true
2. NICK alice      → nick = "alice", tryRegister() → rien (manque user)
3. USER alice 0 * :Alice → user = "alice", tryRegister() → registered = true !
```

---

### **Étape 3 : Gestion des canaux (2h) - `src/Handlers_Channel.cpp`**

**Ce fichier contient 5 fonctions. Focus sur les 3 principales :**

#### **3.1 `h_JOIN()` (1h)**
```
Objectif : Rejoindre/créer un canal

Flow :
1. Vérifier que le client est registered
2. Vérifier que le nom est valide (#xxx)
3. Si canal n'existe pas → le créer + créateur devient op
4. Sinon, vérifier les modes :
   - Mode +i → vérifier inviteList
   - Mode +k → vérifier le mot de passe
   - Mode +l → vérifier la limite
5. Ajouter le membre
6. Broadcast le JOIN
7. Envoyer topic + liste des membres
```

**EXERCICE :** Trace ces 2 cas :

**Cas 1 : Créer un canal**
```
alice (fd=4) : "JOIN #test"
→ channels["#test"] créé
→ ch.members = {4}
→ ch.operators = {4}  ← alice est op
→ Broadcast : ":alice!alice@host JOIN #test"
```

**Cas 2 : Rejoindre avec mode +k**
```
#test existe avec key = "secret"
bob : "JOIN #test"          → ERR_BADCHANNELKEY
bob : "JOIN #test secret"   → OK !
```

---

#### **3.2 `h_PART()` (30min)**
```
Objectif : Quitter un canal

Flow :
1. Vérifier que le canal existe
2. Vérifier qu'on est membre
3. Broadcast le PART (AVANT de retirer)
4. Retirer le membre + retirer des ops si nécessaire
5. Si canal vide → supprimer le canal
```

**QUESTION CLÉ :** Pourquoi on broadcast AVANT de retirer le membre ?

---

#### **3.3 `h_KICK()` (30min)**
```
Objectif : Expulser un membre

Flow :
1. Vérifier que le canal existe
2. Vérifier qu'on est dans le canal
3. Vérifier qu'on est opérateur
4. Vérifier que la cible est membre
5. Broadcast le KICK
6. Retirer la cible
```

**EXERCICE :** Trace :
```
Canal #test : alice (op), bob
alice : "KICK #test bob :bad behavior"
→ Broadcast à alice ET bob : ":alice!alice@host KICK #test bob :bad behavior"
→ bob retiré du canal
```

---

### **Étape 4 : Les MODES (2h) - `src/Handlers_Channel_Modes.cpp`**

**CE FICHIER EST LE PLUS COMPLEXE !**

#### **4.1 Comprendre le principe général (30min)**

**Syntaxe MODE :**
```
MODE #test +i        ← Activer +i
MODE #test -i        ← Désactiver +i
MODE #test +o bob    ← Donner op à bob
MODE #test +k secret ← Mettre un mot de passe
MODE #test +l 10     ← Limite de 10 users
MODE #test +it       ← +i et +t en même temps
```

**Flow général :**
```
1. Parser la chaîne de modes ("+it" ou "-o" etc.)
2. Pour chaque caractère :
   - Si '+' → adding = true
   - Si '-' → adding = false
   - Sinon → traiter le mode
3. Broadcast les modes appliqués
```

---

#### **4.2 Étudier chaque mode (1h30)**

**Mode +i (invite-only) :**
```cpp
case 'i':
    ch.inviteOnly = adding;  // true ou false
    break;
```

**Mode +t (topic restricted) :**
```cpp
case 't':
    ch.topicRestricted = adding;
    break;
```

**Mode +k (key/password) :**
```cpp
case 'k':
    if (adding) {
        ch.key = msg.params[paramIndex++];  // Récupérer le mot de passe
    } else {
        ch.key.clear();  // Supprimer le mot de passe
    }
    break;
```

**Mode +o (operator) :**
```cpp
case 'o':
    std::string targetNick = msg.params[paramIndex++];
    int targetFd = nickToFd[toLower(targetNick)];
    
    if (adding) {
        ch.operators.insert(targetFd);  // Donner op
    } else {
        ch.operators.erase(targetFd);   // Retirer op
    }
    break;
```

**Mode +l (user limit) :**
```cpp
case 'l':
    if (adding) {
        ch.userLimit = atoi(msg.params[paramIndex++].c_str());
    } else {
        ch.userLimit = -1;  // Pas de limite
    }
    break;
```

**EXERCICE :** Trace ces scénarios :

**Scénario 1 : Mode +i**
```
alice (op) : "MODE #test +i"
→ ch.inviteOnly = true
→ Maintenant bob ne peut pas JOIN sans INVITE
```

**Scénario 2 : Mode +o**
```
alice (op) : "MODE #test +o bob"
→ ch.operators.insert(bob_fd)
→ bob peut maintenant faire KICK, MODE, etc.
```

**Scénario 3 : Mode +k**
```
alice (op) : "MODE #test +k secret"
→ ch.key = "secret"
→ bob : "JOIN #test" → ERR_BADCHANNELKEY
→ bob : "JOIN #test secret" → OK !
```

---

### **Étape 5 : Messages et extras (1h) - `src/Handlers_Extras.cpp`**

#### **5.1 `h_PRIVMSG()` (45min)**
```
Objectif : Envoyer un message à un canal ou un utilisateur

Flow :
CAS 1 : Cible = canal (#xxx)
1. Vérifier que le canal existe
2. Vérifier qu'on est membre
3. Broadcast à tous SAUF l'expéditeur

CAS 2 : Cible = utilisateur
1. Vérifier que l'utilisateur existe
2. Envoyer seulement à lui
```

**EXERCICE :** Trace :
```
Canal #test : alice (fd=4), bob (fd=5)

alice : "PRIVMSG #test :hello"
→ bob reçoit : ":alice!alice@host PRIVMSG #test :hello"
→ alice ne reçoit RIEN (pas d'écho)
```

---

#### **5.2 Autres commandes (15min)**

**Parcourir rapidement :**
- `h_INVITE()` : Ajouter un nick à `inviteList`
- `h_TOPIC()` : Consulter/modifier le topic
- `h_NOTICE()` : Comme PRIVMSG mais sans erreur

**Pas besoin d'approfondir, juste comprendre le principe.**

---

### **Étape 6 : Révision et tests (1h)**

#### **6.1 Quiz de contrôle (30min)**

**Question 1 :** Dans quel ordre appeler les commandes pour s'authentifier ?
<details><summary>Réponse</summary>PASS → NICK → USER</details>

**Question 2 :** Qui devient opérateur quand on crée un canal ?
<details><summary>Réponse</summary>Le créateur (premier à faire JOIN)</details>

**Question 3 :** Que fait le mode +i ?
<details><summary>Réponse</summary>Invite-only : seuls les invités peuvent rejoindre</details>

**Question 4 :** Pourquoi on broadcast AVANT de retirer dans PART/KICK ?
<details><summary>Réponse</summary>Pour que le membre concerné reçoive aussi le message</details>

**Question 5 :** Quelle est la différence entre `members` et `operators` ?
<details><summary>Réponse</summary>`members` = tous les utilisateurs du canal, `operators` = ceux qui ont les privilèges</details>

---

#### **6.2 Tester avec nc (30min)**

```bash
# Terminal 1 : Lancer le serveur
./ircserv 6667 secret

# Terminal 2 : alice
nc 127.0.0.1 6667
PASS secret
NICK alice
USER alice 0 * :Alice
JOIN #test
MODE #test +i
INVITE bob #test
PRIVMSG #test :hello everyone

# Terminal 3 : bob
nc 127.0.0.1 6667
PASS secret
NICK bob
USER bob 0 * :Bob
JOIN #test       # Devrait fonctionner (invité)
PRIVMSG #test :hello alice
```

**NOTE CE QUI S'AFFICHE !** C'est ça que l'évaluateur va vérifier.

---

## 📊 **RÉSUMÉ : Ta checklist finale**

### ✅ **Tu dois pouvoir expliquer :**

**Structures :**
- [ ] Contenu de `Client` (fd, nick, user, passwordOk, registered)
- [ ] Contenu de `Channel` (name, members, operators, modes)

**Authentification :**
- [ ] Flow PASS → NICK → USER → tryRegister()
- [ ] Quand `registered` passe à `true`

**Canaux :**
- [ ] Comment on crée un canal (premier JOIN)
- [ ] Qui devient opérateur (le créateur)
- [ ] Les checks de JOIN (+i, +k, +l)
- [ ] Pourquoi broadcast AVANT de retirer

**Modes :**
- [ ] Expliquer chaque mode (+i, +t, +k, +o, +l)
- [ ] Quels modes nécessitent un paramètre (+k, +o, +l)
- [ ] Comment parser "+it" ou "-o bob"

**Messages :**
- [ ] PRIVMSG canal vs PRIVMSG user
- [ ] Pourquoi pas d'écho sur PRIVMSG canal

---

## ⏱️ **Planning ultra-rapide (6h total)**

```
Heure 1   : Client.hpp + Channel.hpp + Numerics.hpp
Heure 2-3 : Handlers_Core.cpp (PASS, NICK, USER)
Heure 4-5 : Handlers_Channel.cpp (JOIN, PART, KICK)
Heure 6-7 : Handlers_Channel_Modes.cpp (les 5 modes)
Heure 8   : Handlers_Extras.cpp + tests avec nc
```

---

## 🚀 **Méthode de travail**

**Pour chaque fichier :**
1. **Lire le code ligne par ligne** (pas copier-coller !)
2. **Écrire en commentaire ce que fait chaque bloc**
3. **Tracer UN scénario complet sur papier**
4. **Tester avec nc pour vérifier**

---

Maintenant dis-moi : **par où veux-tu commencer ?** 

1. Les structures (Client + Channel) ?
2. L'authentification (PASS/NICK/USER) ?
3. Les canaux (JOIN/PART) ?
4. Les modes (le plus complexe) ?

Je te ferai une explication détaillée du fichier que tu choisis ! 🎯










QUESTIONS :

- hasPass (client.hpp) = Pass user or pass server ?
- closed (client.hpp) à quoi ca sert ? 

- Pourquoi -pedantic -MMD -MP (Makefile)