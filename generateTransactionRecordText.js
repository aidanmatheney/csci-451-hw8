(() => {
  const getRandomInt = (/** @type {number} */ minInclusive, /** @type {number} */ maxExclusive) => {
    return Math.floor(Math.random() * (maxExclusive - minInclusive + 1) + minInclusive);
  };
  const getRandomFloat = (/** @type {number} */ minInclusive, /** @type {number} */ maxExclusive) => {
    return Math.random() * (maxExclusive - minInclusive) + minInclusive;
  };

  const range = (/** @type {number} */ start, /** @type {number} */ count) => {
    return Array.from({length: count}).map((_, i) => i + start)
  };

  const criticalSectionCount = getRandomInt(5, 10);
  const criticalSections = range(0, criticalSectionCount).map(() => {
    const transactionCount = getRandomInt(1, 7);
    const transactions = range(0, transactionCount).map(() => {
      const difference = getRandomFloat(-500, 500);
      return `${difference >= 0 ? '+' : ''}${difference.toFixed(2)}\n`;
    });

    return `R\n${transactions.join('')}W\n`
  });

  return criticalSections.join('');
})();
