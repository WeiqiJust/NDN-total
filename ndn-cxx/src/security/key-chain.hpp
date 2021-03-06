/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2014 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Yingdi Yu <http://irl.cs.ucla.edu/~yingdi/>
 */

#ifndef NDN_SECURITY_KEY_CHAIN_HPP
#define NDN_SECURITY_KEY_CHAIN_HPP

#include "sec-public-info.hpp"
#include "sec-tpm.hpp"
#include "key-params.hpp"
#include "secured-bag.hpp"
#include "signature-sha256-with-rsa.hpp"
#include "signature-sha256-with-ecdsa.hpp"
#include "digest-sha256.hpp"

#include "../interest.hpp"
#include "../util/crypto.hpp"
#include "../util/random.hpp"


namespace ndn {

template<class TypePib, class TypeTpm>
class KeyChainTraits
{
public:
  typedef TypePib Pib;
  typedef TypeTpm Tpm;
};

class KeyChain : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  static const Name DEFAULT_PREFIX;

  // RsaKeyParams is set to be default for backward compatibility.
  static const RsaKeyParams DEFAULT_KEY_PARAMS;

  KeyChain();

  template<class KeyChainTraits>
  explicit
  KeyChain(KeyChainTraits traits);

  KeyChain(const std::string& pibName,
           const std::string& tpmName);

  virtual
  ~KeyChain();

  /**
   * @brief Create an identity by creating a pair of Key-Signing-Key (KSK) for this identity and a
   *        self-signed certificate of the KSK.
   *
   * @param identityName The name of the identity.
   * @param params The key parameter if a key needs to be generated for the identity.
   * @return The name of the default certificate of the identity.
   */
  Name
  createIdentity(const Name& identityName, const KeyParams& params = DEFAULT_KEY_PARAMS);

  /**
   * @brief Generate a pair of RSA keys for the specified identity.
   *
   * @param identityName The name of the identity.
   * @param isKsk true for generating a Key-Signing-Key (KSK), false for a Data-Signing-Key (KSK).
   * @param keySize The size of the key.
   * @return The generated key name.
   */
  inline Name
  generateRsaKeyPair(const Name& identityName, bool isKsk = false, uint32_t keySize = 2048);

  inline Name
  generateEcdsaKeyPair(const Name& identityName, bool isKsk = false, uint32_t keySize = 256);
  /**
   * @brief Generate a pair of RSA keys for the specified identity and set it as default key for
   *        the identity.
   *
   * @param identityName The name of the identity.
   * @param isKsk true for generating a Key-Signing-Key (KSK), false for a Data-Signing-Key (KSK).
   * @param keySize The size of the key.
   * @return The generated key name.
   */
  Name
  generateRsaKeyPairAsDefault(const Name& identityName, bool isKsk = false,
                              uint32_t keySize = 2048);

  Name
  generateEcdsaKeyPairAsDefault(const Name& identityName, bool isKsk, uint32_t keySize = 256);

  /**
   * @brief prepare an unsigned identity certificate
   *
   * @param keyName Key name, e.g., `/<identity_name>/ksk-123456`.
   * @param signingIdentity The signing identity.
   * @param notBefore Refer to IdentityCertificate.
   * @param notAfter Refer to IdentityCertificate.
   * @param subjectDescription Refer to IdentityCertificate.
   * @param certPrefix Prefix before `KEY` component. By default, KeyChain will infer the
   *                   certificate name according to the relation between the signingIdentity and
   *                   the subject identity. If signingIdentity is a prefix of the subject identity,
   *                   `KEY` will be inserted after the signingIdentity, otherwise `KEY` is inserted
   *                   after subject identity (i.e., before `ksk-....`).
   * @return IdentityCertificate.
   */
  shared_ptr<IdentityCertificate>
  prepareUnsignedIdentityCertificate(const Name& keyName,
    const Name& signingIdentity,
    const time::system_clock::TimePoint& notBefore,
    const time::system_clock::TimePoint& notAfter,
    const std::vector<CertificateSubjectDescription>& subjectDescription,
    const Name& certPrefix = DEFAULT_PREFIX);

  /**
   * @brief prepare an unsigned identity certificate
   *
   * @param keyName Key name, e.g., `/<identity_name>/ksk-123456`.
   * @param publicKey Public key to sign.
   * @param signingIdentity The signing identity.
   * @param notBefore Refer to IdentityCertificate.
   * @param notAfter Refer to IdentityCertificate.
   * @param subjectDescription Refer to IdentityCertificate.
   * @param certPrefix Prefix before `KEY` component. By default, KeyChain will infer the
   *                   certificate name according to the relation between the signingIdentity and
   *                   the subject identity. If signingIdentity is a prefix of the subject identity,
   *                   `KEY` will be inserted after the signingIdentity, otherwise `KEY` is inserted
   *                   after subject identity (i.e., before `ksk-....`).
   * @return IdentityCertificate.
   */
  shared_ptr<IdentityCertificate>
  prepareUnsignedIdentityCertificate(const Name& keyName,
    const PublicKey& publicKey,
    const Name& signingIdentity,
    const time::system_clock::TimePoint& notBefore,
    const time::system_clock::TimePoint& notAfter,
    const std::vector<CertificateSubjectDescription>& subjectDescription,
    const Name& certPrefix = DEFAULT_PREFIX);

  /**
   * @brief Sign packet with default identity
   *
   * On return, signatureInfo and signatureValue in the packet are set.
   * If default identity does not exist,
   * a temporary identity will be created and set as default.
   *
   * @param packet The packet to be signed
   */
  template<typename T>
  void
  sign(T& packet);

  /**
   * @brief Sign packet with a particular certificate.
   *
   * @param packet The packet to be signed.
   * @param certificateName The certificate name of the key to use for signing.
   * @throws SecPublicInfo::Error if certificate does not exist.
   */
  template<typename T>
  void
  sign(T& packet, const Name& certificateName);

  /**
   * @brief Sign the byte array using a particular certificate.
   *
   * @param buffer The byte array to be signed.
   * @param bufferLength the length of buffer.
   * @param certificateName The certificate name of the signing key.
   * @return The Signature.
   * @throws SecPublicInfo::Error if certificate does not exist.
   */
  Signature
  sign(const uint8_t* buffer, size_t bufferLength, const Name& certificateName);

  /**
   * @brief Sign packet using the default certificate of a particular identity.
   *
   * If there is no default certificate of that identity, this method will create a self-signed
   * certificate.
   *
   * @param packet The packet to be signed.
   * @param identityName The signing identity name.
   */
  template<typename T>
  void
  signByIdentity(T& packet, const Name& identityName);

  /**
   * @brief Sign the byte array using the default certificate of a particular identity.
   *
   * @param buffer The byte array to be signed.
   * @param bufferLength the length of buffer.
   * @param identityName The identity name.
   * @return The Signature.
   */
  Signature
  signByIdentity(const uint8_t* buffer, size_t bufferLength, const Name& identityName);

  /**
   * @brief Set Sha256 weak signature for @p data
   */
  void
  signWithSha256(Data& data);

  /**
   * @brief Set Sha256 weak signature for @p interest
   */
  void
  signWithSha256(Interest& interest);

  /**
   * @brief Generate a self-signed certificate for a public key.
   *
   * @param keyName The name of the public key
   * @return The generated certificate, shared_ptr<IdentityCertificate>() if selfSign fails
   */
  shared_ptr<IdentityCertificate>
  selfSign(const Name& keyName);

  /**
   * @brief Self-sign the supplied identity certificate.
   *
   * @param cert The supplied cert.
   * @throws SecTpm::Error if the private key does not exist.
   */
  void
  selfSign(IdentityCertificate& cert);

  /**
   * @brief delete a certificate.
   *
   * If the certificate to be deleted is current default system default,
   * the method will not delete the certificate and return immediately.
   *
   * @param certificateName The certificate to be deleted.
   */
  void
  deleteCertificate(const Name& certificateName);

  /**
   * @brief delete a key.
   *
   * If the key to be deleted is current default system default,
   * the method will not delete the key and return immediately.
   *
   * @param keyName The key to be deleted.
   */
  void
  deleteKey(const Name& keyName);

  /**
   * @brief delete an identity.
   *
   * If the identity to be deleted is current default system default,
   * the method will not delete the identity and return immediately.
   *
   * @param identity The identity to be deleted.
   */
  void
  deleteIdentity(const Name& identity);

  /**
   * @brief export an identity.
   *
   * @param identity The identity to export.
   * @param passwordStr The password to secure the private key.
   * @return The encoded export data.
   * @throws SecPublicInfo::Error if anything goes wrong in exporting.
   */
  shared_ptr<SecuredBag>
  exportIdentity(const Name& identity, const std::string& passwordStr);

  /**
   * @brief import an identity.
   *
   * @param securedBag The encoded import data.
   * @param passwordStr The password to secure the private key.
   */
  void
  importIdentity(const SecuredBag& securedBag, const std::string& passwordStr);

  SecPublicInfo&
  getPib()
  {
    return *m_pib;
  }

  const SecPublicInfo&
  getPib() const
  {
    return *m_pib;
  }

  SecTpm&
  getTpm()
  {
    return *m_tpm;
  }

  const SecTpm&
  getTpm() const
  {
    return *m_tpm;
  }

  /*******************************
   *  Wrapper of SecPublicInfo   *
   *******************************/
  bool
  doesIdentityExist(const Name& identityName) const
  {
    return m_pib->doesIdentityExist(identityName);
  }

  void
  addIdentity(const Name& identityName)
  {
    return m_pib->addIdentity(identityName);
  }

  bool
  doesPublicKeyExist(const Name& keyName) const
  {
    return m_pib->doesPublicKeyExist(keyName);
  }

  void
  addPublicKey(const Name& keyName, KeyType keyType, const PublicKey& publicKeyDer)
  {
    return m_pib->addPublicKey(keyName, keyType, publicKeyDer);
  }

  void
  addKey(const Name& keyName, const PublicKey& publicKeyDer)
  {
    return m_pib->addKey(keyName, publicKeyDer);
  }

  shared_ptr<PublicKey>
  getPublicKey(const Name& keyName) const
  {
    return m_pib->getPublicKey(keyName);
  }

  bool
  doesCertificateExist(const Name& certificateName) const
  {
    return m_pib->doesCertificateExist(certificateName);
  }

  void
  addCertificate(const IdentityCertificate& certificate)
  {
    return m_pib->addCertificate(certificate);
  }

  shared_ptr<IdentityCertificate>
  getCertificate(const Name& certificateName) const
  {
    return m_pib->getCertificate(certificateName);
  }

  Name
  getDefaultIdentity() const
  {
    return m_pib->getDefaultIdentity();
  }

  Name
  getDefaultKeyNameForIdentity(const Name& identityName) const
  {
    return m_pib->getDefaultKeyNameForIdentity(identityName);
  }

  Name
  getDefaultCertificateNameForKey(const Name& keyName) const
  {
    return m_pib->getDefaultCertificateNameForKey(keyName);
  }

  void
  getAllIdentities(std::vector<Name>& nameList, bool isDefault) const
  {
    return m_pib->getAllIdentities(nameList, isDefault);
  }

  void
  getAllKeyNames(std::vector<Name>& nameList, bool isDefault) const
  {
    return m_pib->getAllKeyNames(nameList, isDefault);
  }

  void
  getAllKeyNamesOfIdentity(const Name& identity, std::vector<Name>& nameList, bool isDefault) const
  {
    return m_pib->getAllKeyNamesOfIdentity(identity, nameList, isDefault);
  }

  void
  getAllCertificateNames(std::vector<Name>& nameList, bool isDefault) const
  {
    return m_pib->getAllCertificateNames(nameList, isDefault);
  }

  void
  getAllCertificateNamesOfKey(const Name& keyName,
                              std::vector<Name>& nameList,
                              bool isDefault) const
  {
    return m_pib->getAllCertificateNamesOfKey(keyName, nameList, isDefault);
  }

  void
  deleteCertificateInfo(const Name& certificateName)
  {
    return m_pib->deleteCertificateInfo(certificateName);
  }

  void
  deletePublicKeyInfo(const Name& keyName)
  {
    return m_pib->deletePublicKeyInfo(keyName);
  }

  void
  deleteIdentityInfo(const Name& identity)
  {
    return m_pib->deleteIdentityInfo(identity);
  }

  void
  setDefaultIdentity(const Name& identityName)
  {
    return m_pib->setDefaultIdentity(identityName);
  }

  void
  setDefaultKeyNameForIdentity(const Name& keyName)
  {
    return m_pib->setDefaultKeyNameForIdentity(keyName);
  }

  void
  setDefaultCertificateNameForKey(const Name& certificateName)
  {
    return m_pib->setDefaultCertificateNameForKey(certificateName);
  }

  Name
  getNewKeyName(const Name& identityName, bool useKsk)
  {
    return m_pib->getNewKeyName(identityName, useKsk);
  }

  Name
  getDefaultCertificateNameForIdentity(const Name& identityName) const
  {
    return m_pib->getDefaultCertificateNameForIdentity(identityName);
  }

  Name
  getDefaultCertificateName() const
  {
    return m_pib->getDefaultCertificateName();
  }

  void
  addCertificateAsKeyDefault(const IdentityCertificate& certificate)
  {
    return m_pib->addCertificateAsKeyDefault(certificate);
  }

  void
  addCertificateAsIdentityDefault(const IdentityCertificate& certificate)
  {
    return m_pib->addCertificateAsIdentityDefault(certificate);
  }

  void
  addCertificateAsSystemDefault(const IdentityCertificate& certificate)
  {
    return m_pib->addCertificateAsSystemDefault(certificate);
  }

  shared_ptr<IdentityCertificate>
  getDefaultCertificate() const
  {
    if (!static_cast<bool>(m_pib->getDefaultCertificate()))
      const_cast<KeyChain*>(this)->setDefaultCertificateInternal();

    return m_pib->getDefaultCertificate();
  }

  void
  refreshDefaultCertificate()
  {
    return m_pib->refreshDefaultCertificate();
  }

  /*******************************
   *  Wrapper of SecTpm          *
   *******************************/

  void
  setTpmPassword(const uint8_t* password, size_t passwordLength)
  {
    return m_tpm->setTpmPassword(password, passwordLength);
  }

  void
  resetTpmPassword()
  {
    return m_tpm->resetTpmPassword();
  }

  void
  setInTerminal(bool inTerminal)
  {
    return m_tpm->setInTerminal(inTerminal);
  }

  bool
  getInTerminal() const
  {
    return m_tpm->getInTerminal();
  }

  bool
  isLocked() const
  {
    return m_tpm->isLocked();
  }

  bool
  unlockTpm(const char* password, size_t passwordLength, bool usePassword)
  {
    return m_tpm->unlockTpm(password, passwordLength, usePassword);
  }

  void
  generateKeyPairInTpm(const Name& keyName, const KeyParams& params)
  {
    return m_tpm->generateKeyPairInTpm(keyName, params);
  }

  void
  deleteKeyPairInTpm(const Name& keyName)
  {
    return m_tpm->deleteKeyPairInTpm(keyName);
  }

  shared_ptr<PublicKey>
  getPublicKeyFromTpm(const Name& keyName) const
  {
    return m_tpm->getPublicKeyFromTpm(keyName);
  }

  Block
  signInTpm(const uint8_t* data, size_t dataLength,
            const Name& keyName,
            DigestAlgorithm digestAlgorithm)
  {
    return m_tpm->signInTpm(data, dataLength, keyName, digestAlgorithm);
  }

  ConstBufferPtr
  decryptInTpm(const uint8_t* data, size_t dataLength, const Name& keyName, bool isSymmetric)
  {
    return m_tpm->decryptInTpm(data, dataLength, keyName, isSymmetric);
  }

  ConstBufferPtr
  encryptInTpm(const uint8_t* data, size_t dataLength, const Name& keyName, bool isSymmetric)
  {
    return m_tpm->encryptInTpm(data, dataLength, keyName, isSymmetric);
  }

  void
  generateSymmetricKeyInTpm(const Name& keyName, const KeyParams& params)
  {
    return m_tpm->generateSymmetricKeyInTpm(keyName, params);
  }

  bool
  doesKeyExistInTpm(const Name& keyName, KeyClass keyClass) const
  {
    return m_tpm->doesKeyExistInTpm(keyName, keyClass);
  }

  bool
  generateRandomBlock(uint8_t* res, size_t size) const
  {
    return m_tpm->generateRandomBlock(res, size);
  }

  void
  addAppToAcl(const Name& keyName, KeyClass keyClass, const std::string& appPath, AclType acl)
  {
    return m_tpm->addAppToAcl(keyName, keyClass, appPath, acl);
  }

  ConstBufferPtr
  exportPrivateKeyPkcs5FromTpm(const Name& keyName, const std::string& password)
  {
    return m_tpm->exportPrivateKeyPkcs5FromTpm(keyName, password);
  }

  bool
  importPrivateKeyPkcs5IntoTpm(const Name& keyName,
                               const uint8_t* buf, size_t size,
                               const std::string& password)
  {
    return m_tpm->importPrivateKeyPkcs5IntoTpm(keyName, buf, size, password);
  }

private:
  /**
   * @brief Determine signature type
   *
   * An empty pointer will be returned if there is no valid signature.
   */
  shared_ptr<Signature>
  determineSignatureWithPublicKey(const KeyLocator& keyLocator,
                                  KeyType keyType,
                                  DigestAlgorithm digestAlgorithm = DIGEST_ALGORITHM_SHA256);

  /**
   * @brief Set default certificate if it is not initialized
   */
  void
  setDefaultCertificateInternal();

  /**
   * @brief Sign a packet using a pariticular certificate.
   *
   * @param packet The packet to be signed.
   * @param certificate The signing certificate.
   */
  template<typename T>
  void
  sign(T& packet, const IdentityCertificate& certificate);

  /**
   * @brief Generate a key pair for the specified identity.
   *
   * @param identityName The name of the specified identity.
   * @param isKsk true for generating a Key-Signing-Key (KSK), false for a Data-Signing-Key (KSK).
   * @param params The parameter of the key.
   * @return The name of the generated key.
   */
  Name
  generateKeyPair(const Name& identityName, bool isKsk = false,
                  const KeyParams& params = DEFAULT_KEY_PARAMS);

  /**
   * @brief Sign the data using a particular key.
   *
   * @param data Reference to the data packet.
   * @param signature Signature to be added.
   * @param keyName The name of the signing key.
   * @param digestAlgorithm the digest algorithm.
   * @throws Tpm::Error
   */
  void
  signPacketWrapper(Data& data, const Signature& signature,
                    const Name& keyName, DigestAlgorithm digestAlgorithm);

  /**
   * @brief Sign the interest using a particular key.
   *
   * @param interest Reference to the interest packet.
   * @param signature Signature to be added.
   * @param keyName The name of the signing key.
   * @param digestAlgorithm the digest algorithm.
   * @throws Tpm::Error
   */
  void
  signPacketWrapper(Interest& interest, const Signature& signature,
                    const Name& keyName, DigestAlgorithm digestAlgorithm);


private:
  SecPublicInfo* m_pib;
  SecTpm* m_tpm;
  time::milliseconds m_lastTimestamp;
};

template<class T>
inline
KeyChain::KeyChain(T)
  : m_pib(new typename T::Pib)
  , m_tpm(new typename T::Tpm)
  , m_lastTimestamp(time::toUnixTimestamp(time::system_clock::now()))
{
}

inline Name
KeyChain::generateRsaKeyPair(const Name& identityName, bool isKsk, uint32_t keySize)
{
  RsaKeyParams params(keySize);
  return generateKeyPair(identityName, isKsk, params);
}

inline Name
KeyChain::generateEcdsaKeyPair(const Name& identityName, bool isKsk, uint32_t keySize)
{
  EcdsaKeyParams params(keySize);
  return generateKeyPair(identityName, isKsk, params);
}

template<typename T>
void
KeyChain::sign(T& packet)
{
  if (!static_cast<bool>(m_pib->getDefaultCertificate()))
    setDefaultCertificateInternal();

  sign(packet, *m_pib->getDefaultCertificate());
}

template<typename T>
void
KeyChain::sign(T& packet, const Name& certificateName)
{
  shared_ptr<IdentityCertificate> certificate = m_pib->getCertificate(certificateName);
  sign(packet, *certificate);
}

template<typename T>
void
KeyChain::signByIdentity(T& packet, const Name& identityName)
{
  Name signingCertificateName;
  try
    {
      signingCertificateName = m_pib->getDefaultCertificateNameForIdentity(identityName);
    }
  catch (SecPublicInfo::Error& e)
    {
      signingCertificateName = createIdentity(identityName);
      // Ideally, no exception will be thrown out, unless something goes wrong in the TPM, which
      // is a fatal error.
    }

  // We either get or create the signing certificate, sign packet! (no exception unless fatal
  // error in TPM)
  sign(packet, signingCertificateName);
}

template<typename T>
void
KeyChain::sign(T& packet, const IdentityCertificate& certificate)
{
  KeyLocator keyLocator(certificate.getName().getPrefix(-1));

  shared_ptr<Signature> signature =
    determineSignatureWithPublicKey(keyLocator, certificate.getPublicKeyInfo().getKeyType());

  if (!static_cast<bool>(signature))
    throw SecPublicInfo::Error("unknown key type!");

  signPacketWrapper(packet, *signature,
                    certificate.getPublicKeyName(),
                    DIGEST_ALGORITHM_SHA256);

  return;
}

} // namespace ndn

#endif // NDN_SECURITY_KEY_CHAIN_HPP
