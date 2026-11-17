#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "JNSkeletalRefitJigFactory.generated.h"

/** 在内容浏览器里新建 Skeletal Refit 对应表，默认名 JIG_NewSkeletalRefit。 */
UCLASS()
class UJNSkeletalRefitJigFactory : public UFactory
{
	GENERATED_BODY()

public:
	UJNSkeletalRefitJigFactory();

	//~ Begin UFactory Interface.
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FText GetDisplayName() const override;
	virtual FString GetDefaultNewAssetName() const override;
	//~ End UFactory Interface.
};
