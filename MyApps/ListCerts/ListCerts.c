//
//  Copyright (c) 2012-2019  Finnbarr P. Murphy.  All rights reserved.
//
//  Display UEFI Secure Boot keys and certificates (PK, KEK, db, dbx)
// 
//  License: BSD 2 clause License
//

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Guid/GlobalVariable.h>
#include <Guid/WinCertificate.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/LoadedImage.h>

#include "oid_registry.h"
#include "x509.h"
#include "asn1_ber_decoder.h"

#define UTCDATE_LEN 23
#define UTILITY_VERSION L"20190403"
#undef DEBUG

// temporary store for output text
CHAR16 tmpbuf[1000];
int    wrapno = 1;


CHAR16 *
AsciiToUnicode( const char *Str, 
                int Len )
{
    CHAR16 *Ret = NULL;

    Ret = AllocateZeroPool(Len*2 + 2);
    if (!Ret)
        return NULL;

    for (int i = 0; i < Len; i++)
        Ret[i] = Str[i];

    return Ret;
}


int 
do_version( void *context, 
            long state_index,
            unsigned char tag,
            const void *value, 
            long vlen )
{
    int Version = *(const char *)value;

    Print(L"  Version: %d (0x%02x) ", Version + 1, Version);

    return 0;
}


int
do_signature( void *context, 
              long state_index,
              unsigned char tag,
              const void *value,
              long vlen )
{
    Print(L"  Signature Algorithm: %s\n", tmpbuf);
    tmpbuf[0] = '\0';

    return 0;
}


int
do_algorithm( void *context,
              long state_index,
              unsigned char tag,
              const void *value, 
              long vlen )
{
    enum OID oid; 
    CHAR16 buffer[100];

    oid = Lookup_OID(value, vlen);
    Sprint_OID(value, vlen, buffer, sizeof(buffer));
    if (oid == OID_id_dsa_with_sha1)
        StrCpy(tmpbuf, L"id_dsa_with_sha1");
    else if (oid == OID_id_dsa)
        StrCpy(tmpbuf, L"id_dsa");
    else if (oid == OID_id_ecdsa_with_sha1)
        StrCpy(tmpbuf, L"id_ecdsa_with_sha1");
    else if (oid == OID_id_ecPublicKey)
        StrCpy(tmpbuf, L"id_ecPublicKey");
    else if (oid == OID_rsaEncryption)
        StrCpy(tmpbuf, L"rsaEncryption");
    else if (oid == OID_md2WithRSAEncryption)
        StrCpy(tmpbuf, L"md2WithRSAEncryption");
    else if (oid == OID_md3WithRSAEncryption)
        StrCpy(tmpbuf, L"md3WithRSAEncryption");
    else if (oid == OID_md4WithRSAEncryption)
        StrCpy(tmpbuf, L"md4WithRSAEncryption");
    else if (oid == OID_sha1WithRSAEncryption)
        StrCpy(tmpbuf, L"sha1WithRSAEncryption");
    else if (oid == OID_sha256WithRSAEncryption)
        StrCpy(tmpbuf, L"sha256WithRSAEncryption");
    else if (oid == OID_sha384WithRSAEncryption)
        StrCpy(tmpbuf, L"sha384WithRSAEncryption");
    else if (oid == OID_sha512WithRSAEncryption)
        StrCpy(tmpbuf, L"sha512WithRSAEncryption");
    else if (oid == OID_sha224WithRSAEncryption)
        StrCpy(tmpbuf, L"sha224WithRSAEncryption");
    else {
        StrCat(tmpbuf, L" (");
        StrCat(tmpbuf, buffer);
        StrCat(tmpbuf, L")");
    }

    return 0;
}


int 
do_serialnumber( void *context, 
                 long state_index,
                 unsigned char tag,
                 const void *value, 
                 long vlen )
{
    char *p = (char *)value;

    Print(L"  Serial Number: ");
    if (vlen > 4) {
        for (int i = 0; i < vlen; i++, p++) {
            Print(L"%02x%c", (UINT8)*p, ((i+1 == vlen)?' ':':'));
        }
    }
    Print(L"\n");

    return 0;
}


int
do_issuer( void *context,
           long  state_index,
           unsigned char tag,
           const void *value,
           long vlen )
{
    Print(L"  Issuer:%s\n", tmpbuf);
    tmpbuf[0] = '\0';

    return 0;
}


int
do_subject( void *context,
            long state_index,
            unsigned char tag,
            const void *value,
            long vlen )
{
    Print(L"  Subject:%s\n", tmpbuf);
    tmpbuf[0] = '\0';

    return 0;
}


int
do_attribute_type( void *context,
                   long state_index,
                   unsigned char tag,
                   const void *value,
                   long vlen )
{
    enum OID oid; 
    CHAR16 buffer[60];

    oid = Lookup_OID(value, vlen);
    Sprint_OID(value, vlen, buffer, sizeof(buffer));
   
    if (oid == OID_countryName) {
        StrCat(tmpbuf, L" C=");
    } else if (oid == OID_stateOrProvinceName) {
        StrCat(tmpbuf, L" ST=");
    } else if (oid == OID_locality) {
        StrCat(tmpbuf, L" L=");
    } else if (oid == OID_organizationName) {
        StrCat(tmpbuf, L" O=");
    } else if (oid == OID_commonName) {
        StrCat(tmpbuf, L" CN=");
    } else {
        StrCat(tmpbuf, L" (");
        StrCat(tmpbuf, buffer);
        StrCat(tmpbuf, L")");
    }

    return 0;
}


int
do_attribute_value( void *context,
                    long state_index,
                    unsigned char tag,
                    const void *value,
                    long vlen )
{
    CHAR16 *ptr;

    ptr = AsciiToUnicode(value, (int)vlen);
    StrCat(tmpbuf, ptr);
    FreePool(ptr);

    return 0;
}


int
do_extensions( void *context,
               long state_index,
               unsigned char tag,
               const void *value,
               long vlen )
{
    Print(L"  Extensions:%s\n", tmpbuf);
    tmpbuf[0] = '\0';
    wrapno = 1;

    return 0;
}


int
do_extension_id( void *context, 
                 long state_index,
                 unsigned char tag,
                 const void *value,
                 long vlen )
{
    enum OID oid; 
    CHAR16 buffer[60];
    int len = StrLen(tmpbuf);

    if (len > (90*wrapno)) {
        // Not sure why a CR is now required in UDK2017.  Need to investigate
        StrCat(tmpbuf, L"\r\n             ");
        wrapno++;
    }

    oid = Lookup_OID(value, vlen);
    Sprint_OID(value, vlen, buffer, sizeof(buffer));

    if (oid == OID_subjectKeyIdentifier)
        StrCat(tmpbuf, L" SubjectKeyIdentifier");
    else if (oid == OID_keyUsage)
        StrCat(tmpbuf, L" KeyUsage");
    else if (oid == OID_subjectAltName)
        StrCat(tmpbuf, L" SubjectAltName");
    else if (oid == OID_issuerAltName)
        StrCat(tmpbuf, L" IssuerAltName");
    else if (oid == OID_basicConstraints)
        StrCat(tmpbuf, L" BasicConstraints");
    else if (oid == OID_crlDistributionPoints)
        StrCat(tmpbuf, L" CrlDistributionPoints");
    else if (oid == OID_certAuthInfoAccess) 
        StrCat(tmpbuf, L" CertAuthInfoAccess");
    else if (oid == OID_certPolicies)
        StrCat(tmpbuf, L" CertPolicies");
    else if (oid == OID_authorityKeyIdentifier)
        StrCat(tmpbuf, L" AuthorityKeyIdentifier");
    else if (oid == OID_extKeyUsage)
        StrCat(tmpbuf, L" ExtKeyUsage");
    else if (oid == OID_msEnrollCerttypeExtension)
        StrCat(tmpbuf, L" msEnrollCertTypeExtension");
    else if (oid == OID_msCertsrvCAVersion)
        StrCat(tmpbuf, L" msCertsrvCAVersion");
    else if (oid == OID_msCertsrvPreviousCertHash)
        StrCat(tmpbuf, L" msCertsrvPreviousCertHash");
    else {
        StrCat(tmpbuf, L" (");
        StrCat(tmpbuf, buffer);
        StrCat(tmpbuf, L")");
    }

    return 0;
}


//
//  Yes, a hack but it works!
//
char *
make_utc_date_string( char *s )
{
    static char buffer[50];
    char  *d;

    d = buffer;
    *d++ = '2';      /* year */
    *d++ = '0';
    *d++ = *s++;
    *d++ = *s++;
    *d++ = '-';
    *d++ = *s++;     /* month */
    *d++ = *s++;
    *d++ = '-';
    *d++ = *s++;     /* day */
    *d++ = *s++;
    *d++ = ' ';
    *d++ = *s++;     /* hour */
    *d++ = *s++;
    *d++ = ':';
    *d++ = *s++;     /* minute */
    *d++ = *s++;
    *d++ = ':';
    *d++ = *s++;     /* second */
    *d++ = *s;
    *d++ = ' ';
    *d++ = 'U';
    *d++ = 'T';
    *d++ = 'C';
    *d = '\0';

    return buffer;
}


int
do_validity_not_before( void *context,
                        long state_index,
                        unsigned char tag,
                        const void *value, 
                        long vlen )
{
    CHAR16 *ptr;
    char *p;

    p = make_utc_date_string((char *)value);
    ptr = AsciiToUnicode(p, UTCDATE_LEN);
    Print(L"  Validity:  Not Before: %s", ptr);
    FreePool(ptr);

    return 0;
}


int
do_validity_not_after( void *context,
                       long state_index,
                       unsigned char tag,
                       const void *value,
                       long vlen )
{
    CHAR16 *ptr;
    char *p;

    p = make_utc_date_string((char *)value);
    ptr = AsciiToUnicode(p, UTCDATE_LEN);
    Print(L"   Not After: %s\n", ptr);
    FreePool(ptr);

    return 0;
}


int
do_subject_public_key_info( void *context, 
                            long state_index,
                            unsigned char tag,
                            const void *value, 
                            long vlen )
{
    Print(L"  Subject Public Key Algorithm: %s\n", tmpbuf);
    tmpbuf[0] = '\0';

    return 0;
}


int
PrintCertificates( UINT8 *data, 
                   UINTN len, 
                   CHAR16 *name )
{
    EFI_SIGNATURE_LIST *CertList = (EFI_SIGNATURE_LIST *)data;
    EFI_SIGNATURE_DATA *Cert;
    EFI_GUID gX509 = EFI_CERT_X509_GUID;
    EFI_GUID gPKCS7 = EFI_CERT_TYPE_PKCS7_GUID;
    EFI_GUID gRSA2048 = EFI_CERT_RSA2048_GUID;
    BOOLEAN  CertFound = FALSE;
    CHAR16   *ext;
    UINTN    DataSize = len;
    UINTN    CertCount = 0;
    UINTN    buflen;
    int      status = 0;

    while ((DataSize > 0) && (DataSize >= CertList->SignatureListSize)) {
        CertCount = (CertList->SignatureListSize - CertList->SignatureHeaderSize) / CertList->SignatureSize;
        Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);

        // should all be X509 but just in case...
        if (CompareGuid(&CertList->SignatureType, &gX509) == 0)
            ext = L"X509";
        else if (CompareGuid(&CertList->SignatureType, &gPKCS7) == 0)
            ext = L"PKCS7";
        else if (CompareGuid(&CertList->SignatureType, &gRSA2048) == 0)
            ext = L"RSA2048";
        else 
            ext = L"Unknown";

        for (UINTN Index = 0; Index < CertCount; Index++) {
            if ( CertList->SignatureSize > 100 ) {
                CertFound = TRUE;
                Print(L"\nType: %s  (GUID: %g)\n", ext, &Cert->SignatureOwner);
                buflen  = CertList->SignatureSize-sizeof(EFI_GUID);
                status = asn1_ber_decoder(&x509_decoder, NULL, Cert->SignatureData, buflen);
            }
            Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
        }
        DataSize -= CertList->SignatureListSize;
        CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
    }

    if ( CertFound == FALSE ) {
       Print(L"\nNo certificates found for this database\n");
    }

    return status;
}


EFI_STATUS
get_variable( CHAR16 *Var, 
              UINT8 **Data, 
              UINTN *Len,
              EFI_GUID Owner )
{
    EFI_STATUS Status;

    *Len = 0;

    Status = gRT->GetVariable( Var, &Owner, NULL, Len, NULL );
    if (Status != EFI_BUFFER_TOO_SMALL)
        return Status;

    *Data = AllocateZeroPool( *Len );
    if (!Data)
        return EFI_OUT_OF_RESOURCES;
    
    Status = gRT->GetVariable( Var, &Owner, NULL, Len, *Data );
    if (Status != EFI_SUCCESS) {
        FreePool( *Data );
        *Data = NULL;
    }

    return Status;
}


EFI_STATUS
OutputVariable( CHAR16 *Var, 
                EFI_GUID Owner ) 
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINT8 *Data;
    UINTN Len;

    Status = get_variable( Var, &Data, &Len, Owner );
    if (Status == EFI_SUCCESS) {
        Print(L"\nVARIABLE: %s  (size: %d)\n", Var, Len);
        PrintCertificates( Data, Len, Var );
        FreePool( Data );
    } else if (Status == EFI_NOT_FOUND) {
#ifdef DEBUG
        Print(L"Variable %s not found\n", Var);
#else
        return Status;
#endif
    } else 
        Print(L"ERROR: Failed to get variable %s. Status Code: %d\n", Var, Status);

    return Status;
}


VOID
Usage( BOOLEAN ErrorMsg )
{
    if ( ErrorMsg ) {
        Print(L"ERROR: Unknown option(s).\n");
    }

    Print(L"Usage: ListCerts [ -pk | -kek | -db | -dbx ]\n");
    Print(L"       ListCerts [-V | --version]\n");
}


INTN
EFIAPI
ShellAppMain( UINTN Argc, 
              CHAR16 **Argv )
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_GUID   gSIGDB = EFI_IMAGE_SECURITY_DATABASE_GUID;
    EFI_GUID   owners[] = { EFI_GLOBAL_VARIABLE, EFI_GLOBAL_VARIABLE, gSIGDB, gSIGDB };
    CHAR16     *variables[] = { L"PK", L"KEK", L"db", L"dbx" };

    if (Argc == 1) {
        for (UINT8 i = 0; i < ARRAY_SIZE(owners); i++) {
            Status = OutputVariable(variables[i], owners[i]);
        }
    } else if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h")) { 
            Usage(FALSE);
            return Status;
        } else if (!StrCmp(Argv[1], L"--version") ||
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[1], L"-pk"))  {
            Status = OutputVariable(variables[0], owners[0]);
        } else if (!StrCmp(Argv[1], L"-kek"))  {
            Status = OutputVariable(variables[1], owners[1]);
        } else if (!StrCmp(Argv[1], L"-db"))  {
            Status = OutputVariable(variables[2], owners[2]);
        } else if (!StrCmp(Argv[1], L"-dbx"))  {
            Status = OutputVariable(variables[3], owners[3]);
        } else {
            Usage(TRUE);
        }
    } else if (Argc > 2) {
        Usage(TRUE);
    }

    return Status;
}
